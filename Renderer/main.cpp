#include "main.h"
#include "Application.h"
#include "Model.h"
#include "cubic.h"
#include "plane.h"
#include "cylinder.h"
#include "sphere.h"
#include <ctime>
#include "logger.h" // application logger

#include "Logging/logger.h" // crash-and-squeeze logger

typedef ::CrashAndSqueeze::Logging::Logger PhysicsLogger;
using CrashAndSqueeze::Core::ForcesArray;
using CrashAndSqueeze::Core::PlaneForce;
using CrashAndSqueeze::Core::SphericalRegion;
using CrashAndSqueeze::Core::IRegion;
using CrashAndSqueeze::Core::CylindricalRegion;
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
    const TCHAR *LIGHTING_SHADER_FILENAME = _T("deform+lighting.vsh");
    
    const TCHAR *MESH_FILENAME = _T("ford.x");

    const D3DCOLOR CYLINDER_COLOR = D3DCOLOR_XRGB(100, 150, 255);
    const D3DCOLOR OBSTACLE_COLOR = D3DCOLOR_XRGB(100, 100, 100);

    const D3DXCOLOR NO_DEFORM_COLOR = CYLINDER_COLOR;
    const D3DXCOLOR MAX_DEFORM_COLOR = D3DCOLOR_XRGB(255, 0, 0);
    const D3DXCOLOR MEDIUM_DEFORM_COLOR = (NO_DEFORM_COLOR + MAX_DEFORM_COLOR)/2.0f;

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
    const D3DXCOLOR HIT_REGION_COLOR = D3DCOLOR_RGBA(255, 255, 0, 128);

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
        Model &model;
        IndexArray one_vertex;

    public:
        RepaintReaction(const IndexArray &shape_vertex_indices, Real threshold_distance, Model &model)
            : ShapeDeformationReaction(shape_vertex_indices, threshold_distance), model(model), one_vertex(1)
        {
            one_vertex.push_back(0);
            one_vertex.freeze();
        }

        virtual void invoke(int vertex_index, Real distance)
        {
            UNREFERENCED_PARAMETER(distance);

            model.repaint_vertices(get_shape_vertex_indices(), MEDIUM_DEFORM_COLOR);
            one_vertex[0] = vertex_index;
            model.repaint_vertices(one_vertex, MAX_DEFORM_COLOR);
        }
    };

    class DummyReaction : public ShapeDeformationReaction
    {
    public:
        DummyReaction(const IndexArray &shape_vertex_indices, Real threshold_distance, Model &model) : ShapeDeformationReaction(shape_vertex_indices, threshold_distance) {UNREFERENCED_PARAMETER(model);}
        virtual void invoke(int vertex_index, Real distance) { UNREFERENCED_PARAMETER(vertex_index); UNREFERENCED_PARAMETER(distance);}
    };

    class MessageBoxHitReaction : public HitReaction
    {
    private:
        const TCHAR * message;

    public:
        MessageBoxHitReaction(const IndexArray & shape_vertex_indices, Real velocity_threshold, const TCHAR * message)
            : HitReaction(shape_vertex_indices, velocity_threshold), message(message) {}

        virtual void invoke(int vertex_index, const Vector &velocity)
        {
            UNREFERENCED_PARAMETER(vertex_index);
            UNREFERENCED_PARAMETER(velocity);
            my_message_box(message, _T("Hit Reaction"), MB_OK | MB_ICONINFORMATION);
            this->disable();
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
    try
    {
        Application app(logger);
        app.set_updating_vertices_on_gpu(true);

        VertexShader simple_shader(app.get_device(), VERTEX_DECL_ARRAY, SIMPLE_SHADER_FILENAME);
        VertexShader lighting_shader(app.get_device(), VERTEX_DECL_ARRAY, LIGHTING_SHADER_FILENAME);
        
        // -------------------------- M o d e l -----------------------

        MeshModel car(app.get_device(), lighting_shader, MESH_FILENAME, CYLINDER_COLOR, D3DXVECTOR3(-2, 0, 0));
        Vertex * car_vertices = car.lock_vertex_buffer();
        PointModel low_car(app.get_device(), simple_shader, car_vertices, car.get_vertices_count(), 10, D3DXVECTOR3(0, 0, 0));
        car.unlock_vertex_buffer();
        
        PhysicalModel * phys_mod = app.add_physical_model(car, low_car, Vector(1,0,0));
        if(NULL == phys_mod)
            throw NullPointerError();




        // -------------------------- F o r c e s -----------------------
        ForcesArray empty;
        app.set_forces(empty);

        SphericalRegion hit_region( Vector(1,0.7,0.5), 0.5 );

        // ------------------ V i s u a l i z a t i o n -----------------------
        sphere_vertices = new Vertex[SPHERE_VERTICES];
        sphere_indices = new Index[SPHERE_INDICES];

        sphere(static_cast<float>(hit_region.get_radius()), D3DXVECTOR3(0,0,0), HIT_REGION_COLOR,
               SPHERE_EDGES_PER_DIAMETER, sphere_vertices, sphere_indices);

        Model hit_region_model(app.get_device(),
                               D3DPT_TRIANGLELIST,
                               simple_shader,
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

        for(int i = 0; i < reactions.size(); ++i)
            delete reactions[i];
        
        logger.log("        [Renderer]", "application shutdown");
    }
    catch(RuntimeError &e)
    {
        delete_array(&sphere_indices);
        delete_array(&sphere_vertices);
        for(int i = 0; i < reactions.size(); ++i)
            delete reactions[i];
        
        logger.log("ERROR!! [Renderer]", e.get_log_entry());
        logger.dump_messages();
        const TCHAR *MESSAGE_BOX_TITLE = _T("Renderer error!");
        my_message_box(e.get_message(), MESSAGE_BOX_TITLE, MB_OK | MB_ICONERROR, true);
        return -1;
    }
    return 0;
}
