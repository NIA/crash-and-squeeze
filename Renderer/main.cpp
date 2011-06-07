#include "main.h"
#include "Application.h"
#include "Model.h"
#include "cubic.h"
#include "plane.h"
#include "cylinder.h"
#include "sphere.h"
#include <ctime>
#include "logger.h" // application logger
#include "matrices.h"

#include "Logging/logger.h" // crash-and-squeeze logger

typedef ::CrashAndSqueeze::Logging::Logger PhysicsLogger;
using CrashAndSqueeze::Core::ForcesArray;
using CrashAndSqueeze::Core::PlaneForce;
using CrashAndSqueeze::Core::SphericalRegion;
using CrashAndSqueeze::Core::IRegion;
using CrashAndSqueeze::Core::CylindricalRegion;
using CrashAndSqueeze::Core::BoxRegion;
using CrashAndSqueeze::Core::ShapeDeformationReaction;
using CrashAndSqueeze::Core::HitReaction;
using CrashAndSqueeze::Core::RegionReaction;
using CrashAndSqueeze::Math::Vector;
using CrashAndSqueeze::Math::Real;
using CrashAndSqueeze::Core::IndexArray;
using CrashAndSqueeze::Collections::Array;

namespace
{
    bool DISABLE_MESSAGE_BOXES = false;

    const TCHAR *SIMPLE_SHADER_FILENAME = _T("simple.vsh");
    const TCHAR *DEFORM_SHADER_FILENAME = _T("deform+lighting.vsh");
    const TCHAR *LIGHTING_SHADER_FILENAME = _T("lighting.vsh");
    
    const TCHAR *MESH_FILENAME = _T("ford.x");

    const D3DCOLOR OBJECT_COLOR = D3DCOLOR_XRGB(100, 150, 255);
    const D3DCOLOR OBSTACLE_COLOR = D3DCOLOR_XRGB(100, 100, 100);

    const D3DXCOLOR NO_DEFORM_COLOR = OBJECT_COLOR;
    const D3DXCOLOR MAX_DEFORM_COLOR = D3DCOLOR_XRGB(255, 0, 0);
    const D3DXCOLOR THRESHOLD_COLOR = NO_DEFORM_COLOR;//(NO_DEFORM_COLOR + MAX_DEFORM_COLOR)/2.0f;

    const D3DXCOLOR FRAME_COLOR = D3DCOLOR_XRGB(128, 255, 0);

    const Real      THRESHOLD_DISTANCE = 0.05;

    const Index LOW_EDGES_PER_BASE = 40;
    const Index LOW_EDGES_PER_HEIGHT = 48;
    const Index LOW_EDGES_PER_CAP = 5;
    const Index LOW_CYLINDER_VERTICES = cylinder_vertices_count(LOW_EDGES_PER_BASE, LOW_EDGES_PER_HEIGHT, LOW_EDGES_PER_CAP);
    const DWORD LOW_CYLINDER_INDICES = cylinder_indices_count(LOW_EDGES_PER_BASE, LOW_EDGES_PER_HEIGHT, LOW_EDGES_PER_CAP);

    const Index HIGH_EDGES_PER_BASE = 300; // 100; //
    const Index HIGH_EDGES_PER_HEIGHT = 300; // 120; //
    const Index HIGH_EDGES_PER_CAP = 50; // 40; //
    const Index HIGH_CYLINDER_VERTICES = cylinder_vertices_count(HIGH_EDGES_PER_BASE, HIGH_EDGES_PER_HEIGHT, HIGH_EDGES_PER_CAP);
    const DWORD HIGH_CYLINDER_INDICES = cylinder_indices_count(HIGH_EDGES_PER_BASE, HIGH_EDGES_PER_HEIGHT, HIGH_EDGES_PER_CAP);

    const Index SPHERE_EDGES_PER_DIAMETER = 8;
    const Index SPHERE_VERTICES = sphere_vertices_count(SPHERE_EDGES_PER_DIAMETER);
    const DWORD SPHERE_INDICES = sphere_indices_count(SPHERE_EDGES_PER_DIAMETER);
    const D3DXCOLOR HIT_REGION_COLOR = D3DCOLOR_XRGB(200, 200, 200);

    inline void my_message_box(const TCHAR *message, const TCHAR *caption, UINT type, bool force = false)
    {
        if( ! DISABLE_MESSAGE_BOXES || force )
        {
            MessageBox(NULL, message, caption, type);
        }
    }

#pragma warning( push )
#pragma warning( disable : 4512 )
    class PhysicsLoggerAction : public ::CrashAndSqueeze::Logging::Action
    {
    protected:
        Logger &logger;
    public:
        PhysicsLoggerAction(Logger & logger) : logger(logger) {}
    };

    class PhysLogAction : public PhysicsLoggerAction
    {
    public:
        PhysLogAction(Logger & logger) : PhysicsLoggerAction(logger) {}
        void invoke(const char * message, const char * file, int line)
        {
            logger.log("        [Crash-And-Squeeze]", message, file, line);
        }
    };

    class PhysWarningAction : public PhysicsLoggerAction
    {
    public:
        PhysWarningAction(Logger & logger) : PhysicsLoggerAction(logger) {}
        void invoke(const char * message, const char * file, int line)
        {
            logger.log("WARNING [Crash-And-Squeeze]", message, file, line);
            my_message_box(_T("Physical subsystem warning! See log"), _T("Crash-And-Squeeze warning!"), MB_OK | MB_ICONEXCLAMATION);
        }
    };

    class PhysErrorAction : public PhysicsLoggerAction
    {
    public:
        PhysErrorAction(Logger & logger) : PhysicsLoggerAction(logger) {}
        void invoke(const char * message, const char * file, int line)
        {
            logger.log("ERROR!! [Crash-And-Squeeze]", message, file, line);
            throw PhysicsError();
        }
    };

    class RegionWithVertices
    {
    private:
        IRegion & region;
        IndexArray physical_vertices;
        IndexArray graphical_vertices;

    public:
        RegionWithVertices(IRegion & region, PhysicalModel & phy_model, AbstractModel & gfx_model)
            : region(region)
        {
            for(int i = 0; i < phy_model.get_vertices_num(); ++i)
            {
                if( region.contains(phy_model.get_vertex(i).get_pos()) )
                    physical_vertices.push_back(i);
            }
            Vertex * buffer = gfx_model.lock_vertex_buffer();
            int gfx_vertices_count = static_cast<int>(gfx_model.get_vertices_count());
            for(int i = 0; i < gfx_vertices_count; ++i)
            {
                if( region.contains( d3dxvector_to_math_vector(buffer[i].pos) ) )
                    graphical_vertices.push_back(i);
            }
            gfx_model.unlock_vertex_buffer();
        }

        IRegion & get_region() { return region; }
        IndexArray & get_physical_vertices() { return physical_vertices; }
        IndexArray & get_graphical_vertices() { return graphical_vertices; }
    };

#pragma warning( pop ) 

    // adds values `from`, `from`+1, ..., `to`-1 to array,
    // total `to`-`from` items.
    void add_range(IndexArray &arr, int from, int to, int step = 1)
    {
        for(int i = from; i < to; i += step)
        {
            arr.push_back(i);
        }
    }

    class RepaintReaction : public ShapeDeformationReaction
    {
    private:
        AbstractModel &model;
        Real max_distance;
        RegionWithVertices rgnwv;

    public:
        RepaintReaction(IRegion &region,
                        PhysicalModel &phy_mod,
                        AbstractModel &gfx_mod,
                        Real threshold_distance,
                        Real max_distance)
            : rgnwv(region, phy_mod, gfx_mod),
              ShapeDeformationReaction(rgnwv.get_physical_vertices(), threshold_distance),
              model(gfx_mod), max_distance(max_distance)
        {
        }

        virtual void invoke(int vertex_index, Real distance)
        {
            UNREFERENCED_PARAMETER(vertex_index);

            D3DXCOLOR color;
            Real coeff = (distance - get_threshold_distance())/(max_distance - get_threshold_distance());
            if(coeff > 1)
                coeff = 1;

            D3DXColorLerp(&color, &THRESHOLD_COLOR, &MAX_DEFORM_COLOR, static_cast<float>(coeff));
            model.repaint_vertices(rgnwv.get_graphical_vertices(), color);
        }
    };

    class FrictionHitReaction : public HitReaction
    {
    private:
        Application * app;

    public:
        FrictionHitReaction(const IndexArray & shape_vertex_indices, Real velocity_threshold, Application * app)
            : HitReaction(shape_vertex_indices, velocity_threshold), app(app) {}

        virtual void invoke(int vertex_index, const Vector &velocity)
        {
            UNREFERENCED_PARAMETER(vertex_index);
            UNREFERENCED_PARAMETER(velocity);
            app->set_friction(true);
        }
    };

    class MessageBoxRegionReaction : public RegionReaction
    {
    private:
        const TCHAR * message;

    public:
        MessageBoxRegionReaction(const IndexArray & shape_vertex_indices,
                                 const IRegion & region,
                                 bool reaction_on_entering,
                                 const TCHAR* message)
            : RegionReaction(shape_vertex_indices, region, reaction_on_entering), message(message) {}
        
        virtual void invoke(int vertex_index)
        {
            UNREFERENCED_PARAMETER(vertex_index);

            my_message_box(message, _T("Region Reaction"), MB_OK | MB_ICONINFORMATION);
            this->disable();
        }
    };

    void paint_model(AbstractModel &model)
    {
        static const int COLORS_COUNT = 5;
        static D3DCOLOR colors[COLORS_COUNT] =
        {
            D3DCOLOR_XRGB(0, 0, 255),
            D3DCOLOR_XRGB(0, 150, 0),
            D3DCOLOR_XRGB(150, 255, 0),
            D3DCOLOR_XRGB(255, 255, 0),
            D3DCOLOR_XRGB(0, 255, 255),
        };
        Vertex * vertices = model.lock_vertex_buffer();
        for(unsigned i = 0; i < model.get_vertices_count(); ++i)
        {
            int color_index = vertices[i].clusters_num % COLORS_COUNT;
            vertices[i].color = colors[color_index];
        }
        model.unlock_vertex_buffer();
    };

    template<class Type> void delete_all(Array<Type*> &arr)
    {
        for(int i = 0; i < arr.size(); ++i)
        {
            delete arr[i];
        }
    }
}

INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, INT )
{
    Logger logger("renderer.log", true);
    logger.newline();
    logger.log("        [Renderer]", "application startup");
    
    PhysicsLogger & phys_logger = PhysicsLogger::get_instance();
    
    PhysLogAction phys_log_action(logger);
    PhysWarningAction phys_warn_action(logger);
    PhysErrorAction phys_err_action(logger);
    
    //phys_logger.set_action(PhysicsLogger::LOG, &phys_log_action);
    phys_logger.ignore(PhysicsLogger::LOG);
    phys_logger.set_action(PhysicsLogger::WARNING, &phys_warn_action);
    phys_logger.set_action(PhysicsLogger::ERROR, &phys_err_action);

    srand( static_cast<unsigned>( time(NULL) ) );
    
    Vertex * sphere_vertices = NULL;
    Index * sphere_indices = NULL;
    
    Array<ShapeDeformationReaction*> reactions;
    Array<IRegion*> regions;
    try
    {
        Application app(logger);
        app.set_updating_vertices_on_gpu(true);

        VertexShader simple_shader(app.get_device(), VERTEX_DECL_ARRAY, SIMPLE_SHADER_FILENAME);
        VertexShader deform_shader(app.get_device(), VERTEX_DECL_ARRAY, DEFORM_SHADER_FILENAME);
        VertexShader lighting_shader(app.get_device(), VERTEX_DECL_ARRAY, LIGHTING_SHADER_FILENAME);
        
        // -------------------------- M o d e l -----------------------

#ifdef _DEMO_SIDE
        MeshModel car(app.get_device(), deform_shader, MESH_FILENAME, OBJECT_COLOR, D3DXVECTOR3(-2, 0, -0.0f));
#endif //ifdef _DEMO_SIDE

#ifdef _DEMO_FRONT
        MeshModel car(app.get_device(), deform_shader, MESH_FILENAME, OBJECT_COLOR, D3DXVECTOR3(-5, 0, 0.1f));
        car.set_rotation(rotate_y_matrix(D3DX_PI/2));
#endif //ifdef _DEMO_FRONT

        Vertex * car_vertices = car.lock_vertex_buffer();
        PointModel low_car(app.get_device(), simple_shader, car_vertices, car.get_vertices_count(), 4, D3DXVECTOR3(0, 0, 0));
        car.unlock_vertex_buffer();
        
        PhysicalModel * phys_mod = app.add_physical_model(car, low_car, Vector(6,0,0));
        if(NULL == phys_mod)
            throw NullPointerError();

        // ------------------------- R e a c t i o n s ------------------

        int x_cells, y_cells, z_cells;
        Vector x_step, y_step, z_step;
        Vector box_min, box_max, box_end;

#if defined(_DEMO_SIDE)
        z_cells = 8;
        y_cells = 6;
        box_min = Vector(0.5, 0.2, -1.8);
        box_max = Vector(1.5, 1.9, 1.4);
        y_step = (box_max[1] - box_min[1])/y_cells*Vector(0,1,0);
        z_step = (box_max[2] - box_min[2])/z_cells*Vector(0,0,1);
        box_end = box_min + y_step + z_step;
        box_end[0] = box_max[0];
        for(int iy = 0; iy < y_cells; ++iy)
        {
            for(int iz = 0; iz < z_cells; ++iz)
            {
                BoxRegion * box = new BoxRegion(box_min + iy*y_step + iz*z_step,
                                                box_end + iy*y_step + iz*z_step);
                RepaintReaction * reaction = 
                    new RepaintReaction(*box, *phys_mod, car, 0.12, 0.3);

                phys_mod->add_shape_deformation_reaction(*reaction);
                regions.push_back(box);
                reactions.push_back(reaction);
            }
        }
        z_cells = 8;
        x_cells = 6;
        box_min = Vector(-1.1, 1.9, -1.8);
        box_max = Vector(1.1, 2.1, 1.4);
        x_step = (box_max[0] - box_min[0])/x_cells*Vector(1,0,0);
        z_step = (box_max[2] - box_min[2])/z_cells*Vector(0,0,1);
        box_end = box_min + x_step + z_step;
        box_end[1] = box_max[1];
        for(int ix = 0; ix < x_cells; ++ix)
        {
            for(int iz = 0; iz < z_cells; ++iz)
            {
                BoxRegion * box = new BoxRegion(box_min + ix*x_step + iz*z_step,
                                                box_end + ix*x_step + iz*z_step);
                RepaintReaction * reaction = 
                    new RepaintReaction(*box, *phys_mod, car, 0.12, 0.35);

                phys_mod->add_shape_deformation_reaction(*reaction);
                regions.push_back(box);
                reactions.push_back(reaction);
            }
        }
#elif defined(_DEMO_FRONT)
        z_cells = 4;
        y_cells = 4;
        box_min = Vector(0.8, 0.2, 2);
        box_max = Vector(1.5, 1.1, 2.9);
        y_step = (box_max[1] - box_min[1])/y_cells*Vector(0,1,0);
        z_step = (box_max[2] - box_min[2])/z_cells*Vector(0,0,1);
        box_end = box_min + y_step + z_step;
        box_end[0] = box_max[0];
        for(int iy = 0; iy < y_cells; ++iy)
        {
            for(int iz = 0; iz < z_cells; ++iz)
            {
                BoxRegion * box = new BoxRegion(box_min + iy*y_step + iz*z_step,
                                                box_end + iy*y_step + iz*z_step);
                RepaintReaction * reaction = 
                    new RepaintReaction(*box, *phys_mod, car, 0.12, 0.35);

                phys_mod->add_shape_deformation_reaction(*reaction);
                regions.push_back(box);
                reactions.push_back(reaction);
            }
        }
        
        z_cells = 4;
        x_cells = 6;
        box_min = Vector(-1.1, 1.1, 2.2);
        box_max = Vector(1.1, 1.6, 2.9);
        x_step = (box_max[0] - box_min[0])/x_cells*Vector(1,0,0);
        z_step = (box_max[2] - box_min[2])/z_cells*Vector(0,0,1);
        box_end = box_min + x_step + z_step;
        box_end[1] = box_max[1];
        for(int ix = 0; ix < x_cells; ++ix)
        {
            for(int iz = 0; iz < z_cells; ++iz)
            {
                BoxRegion * box = new BoxRegion(box_min + ix*x_step + iz*z_step,
                                                box_end + ix*x_step + iz*z_step);
                RepaintReaction * reaction = 
                    new RepaintReaction(*box, *phys_mod, car, 0.12, 0.35);

                phys_mod->add_shape_deformation_reaction(*reaction);
                regions.push_back(box);
                reactions.push_back(reaction);
            }
        }
        
        x_cells = 6;
        y_cells = 4;
        box_min = Vector(-1.1, 0.2, 2.85);
        box_max = Vector(1.1, 1.2, 3.3);
        y_step = (box_max[1] - box_min[1])/y_cells*Vector(0,1,0);
        z_step = (box_max[2] - box_min[2])/z_cells*Vector(0,0,1);
        box_end = box_min + x_step + y_step;
        box_end[2] = box_max[2];
        for(int ix = 0; ix < x_cells; ++ix)
        {
            for(int iy = 0; iy < y_cells; ++iy)
            {
                BoxRegion * box = new BoxRegion(box_min + ix*x_step + iy*y_step,
                                                box_end + ix*x_step + iy*y_step);
                RepaintReaction * reaction = 
                    new RepaintReaction(*box, *phys_mod, car, 0.12, 0.35);

                phys_mod->add_shape_deformation_reaction(*reaction);
                regions.push_back(box);
                reactions.push_back(reaction);
            }
        }
#endif

        app.set_friction(false);
#if defined(_DEMO_SIDE)
        SphericalRegion expected_hit( Vector(1, 1.4, -0.5), 0.5 );
#elif defined(_DEMO_FRONT)
        SphericalRegion expected_hit( Vector(0.66, 0.65, 3), 0.5 );
#endif
        RegionWithVertices rwv(expected_hit, *phys_mod, car);
        FrictionHitReaction friction_reaction(rwv.get_physical_vertices(), 0.1, &app);
        phys_mod->add_hit_reaction(friction_reaction);

        // -------------------------- F o r c e s -----------------------
        ForcesArray empty;
        app.set_forces(empty);

#ifdef _DEMO_SIDE
        SphericalRegion hit_region( Vector(2,1.4,-0.5), 0.5 );
#endif //ifdef _DEMO_SIDE
#ifdef _DEMO_FRONT
        SphericalRegion hit_region( Vector(2,0.65,-0.8), 0.5 );
#endif //ifdef _DEMO_FRONT

        // --------------------------- I m p a c t -----------------------
        sphere_vertices = new Vertex[SPHERE_VERTICES];
        sphere_indices = new Index[SPHERE_INDICES];

        sphere(static_cast<float>(hit_region.get_radius()), D3DXVECTOR3(0,0,0), HIT_REGION_COLOR,
               SPHERE_EDGES_PER_DIAMETER, sphere_vertices, sphere_indices);

        Model hit_region_model(app.get_device(),
                               D3DPT_TRIANGLELIST,
                               lighting_shader,
                               sphere_vertices,
                               SPHERE_VERTICES,
                               sphere_indices,
                               SPHERE_INDICES,
                               SPHERE_INDICES/3,
                               math_vector_to_d3dxvector(hit_region.get_center()));
        hit_region_model.set_draw_ccw(true);

        double impact_velocity = 1;
        app.set_impact( hit_region, Vector(0,-impact_velocity,0), Vector(0, 1.15, 0), hit_region_model);

        // -------------------------- G O ! ! ! -----------------------
        app.run();
        delete_array(&sphere_indices);
        delete_array(&sphere_vertices);
        delete_all(regions);
        delete_all(reactions);
        
        logger.log("        [Renderer]", "application shutdown");
    }
    catch(RuntimeError &e)
    {
        delete_array(&sphere_indices);
        delete_array(&sphere_vertices);
        delete_all(regions);
        delete_all(reactions);
        
        logger.log("ERROR!! [Renderer]", e.get_log_entry());
        logger.dump_messages();
        const TCHAR *MESSAGE_BOX_TITLE = _T("Renderer error!");
        my_message_box(e.get_message(), MESSAGE_BOX_TITLE, MB_OK | MB_ICONERROR, true);
        return -1;
    }
    return 0;
}
