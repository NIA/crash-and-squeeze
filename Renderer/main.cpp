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
    bool DISABLE_MESSAGE_BOXES = true;
    bool PAINT_MODEL = true;

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

    const Index OVAL_EDGES_PER_DIAMETER = 30;
    const Index OVAL_VERTICES = sphere_vertices_count(OVAL_EDGES_PER_DIAMETER);
    const DWORD OVAL_INDICES = sphere_indices_count(OVAL_EDGES_PER_DIAMETER);

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
            my_message_box(_T("Physical subsystem warning! See log"), _T("Crash-And-Squeeze warning!"), MB_OK | MB_ICONEXCLAMATION, true);
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

    class Demo
    {
    protected:
        Vertex * low_model_vertices;
        Index * low_model_indices;
        Vertex * high_model_vertices;
        Index * high_model_indices;

        Application & app;
        VertexShader simple_shader;
        VertexShader lighting_shader;

        AbstractModel * low_model;
        AbstractModel * high_model;
        SphericalRegion * hit_region;
        Model * hit_region_model;

        ForcesArray NO_FORCES;

        // Override this to define Demo models, reactions, etc...
        virtual void prepare() = 0;

        PhysicalModel * add_physical_model(AbstractModel * high_model_, AbstractModel *low_model_)
        {
            high_model = high_model_;
            low_model = low_model_;
            PhysicalModel * phys_mod = app.add_model(*high_model, true, low_model);
            if(NULL == phys_mod)
                throw NullPointerError(_T("(Demo) Failed to add physical model!"));
            if(PAINT_MODEL)
                paint_model(*high_model);
            return phys_mod;
        }

        void set_impact(Vector hit_position, Vector hit_rotation_center, Vector hit_velocity)
        {
            hit_region = new SphericalRegion( hit_position, 0.25 );

            // ------------------ V i s u a l i z a t i o n -----------------------
            Vertex * sphere_vertices = new Vertex[SPHERE_VERTICES];
            Index * sphere_indices = new Index[SPHERE_INDICES];

            sphere(static_cast<float>(hit_region->get_radius()), D3DXVECTOR3(0,0,0), HIT_REGION_COLOR,
                   SPHERE_EDGES_PER_DIAMETER, sphere_vertices, sphere_indices);

            hit_region_model = new Model(
                app.get_device(),
                D3DPT_TRIANGLELIST,
                simple_shader,
                sphere_vertices,
                SPHERE_VERTICES,
                sphere_indices,
                SPHERE_INDICES,
                SPHERE_INDICES/3,
                math_vector_to_d3dxvector(hit_region->get_center()),
                D3DXVECTOR3(0, 0, 0));
            hit_region_model->set_draw_ccw(true);

            app.set_impact( *hit_region, hit_velocity, hit_rotation_center, *hit_region_model);

            delete_array(&sphere_vertices);
            delete_array(&sphere_indices);
        }

    public:
        Demo(Application & _app, Index low_vertices_count, DWORD low_indices_count, Index high_vertices_count, DWORD high_indices_count)
            : app(_app),
              simple_shader(_app.get_device(), VERTEX_DECL_ARRAY, SIMPLE_SHADER_FILENAME),
              lighting_shader(_app.get_device(), VERTEX_DECL_ARRAY, LIGHTING_SHADER_FILENAME),
              low_model(NULL), high_model(NULL), hit_region_model(NULL), hit_region(NULL)
        {
            low_model_vertices = new Vertex[low_vertices_count];
            low_model_indices = new Index[low_indices_count];
            high_model_vertices = new Vertex[high_vertices_count];
            high_model_indices = new Index[high_indices_count];
        }

        void run()
        {
            // Default prepare: set forces
            app.set_forces(NO_FORCES);

            // Custom prepare (should be overriden)
            prepare();
            
            // GO!
            app.run();
        }

        virtual ~Demo()
        {
            delete_array(&low_model_indices);
            delete_array(&low_model_vertices);
            delete_array(&high_model_indices);
            delete_array(&high_model_vertices);
            delete low_model;
            delete high_model;
            delete hit_region_model;
            delete hit_region;
        }
    };

    class CylinderDemo : public Demo
    {
    public:
        CylinderDemo(Application & _app) : Demo(_app, LOW_CYLINDER_VERTICES, LOW_CYLINDER_INDICES, HIGH_CYLINDER_VERTICES, HIGH_CYLINDER_INDICES)
        {}

        virtual ~CylinderDemo()
        {
            for(int i = 0; i < reactions.size(); ++i)
                delete reactions[i];
        }

    private:
        Array<ShapeDeformationReaction*> reactions;
    
    protected:
        virtual void prepare()
        {
            // == PREPARE CYLINDER DEMO ==

            // - Create models -
            const float cylinder_radius = 0.25;
            const float cylinder_height = 1;
            const float cylinder_z = -cylinder_height/2;

            cylinder( cylinder_radius, cylinder_height, D3DXVECTOR3(0,0,cylinder_z),
                     &CYLINDER_COLOR, 1,
                     HIGH_EDGES_PER_BASE, HIGH_EDGES_PER_HEIGHT, HIGH_EDGES_PER_CAP,
                     high_model_vertices, high_model_indices );
            Model * high_cylinder_model = new Model(
                app.get_device(),
                D3DPT_TRIANGLESTRIP,
                lighting_shader,
                high_model_vertices,
                HIGH_CYLINDER_VERTICES,
                high_model_indices,
                HIGH_CYLINDER_INDICES,
                HIGH_CYLINDER_INDICES - 2,
                D3DXVECTOR3(0, 0, 0),
                D3DXVECTOR3(0, 0, 0));
            cylinder( cylinder_radius, cylinder_height, D3DXVECTOR3(0,0,cylinder_z),
                     &CYLINDER_COLOR, 1,
                     LOW_EDGES_PER_BASE, LOW_EDGES_PER_HEIGHT, LOW_EDGES_PER_CAP,
                     low_model_vertices, low_model_indices );
            Model * low_cylinder_model = new Model(
                app.get_device(),
                D3DPT_TRIANGLESTRIP,
                simple_shader,
                low_model_vertices,
                LOW_CYLINDER_VERTICES,
                low_model_indices,
                LOW_CYLINDER_INDICES,
                LOW_CYLINDER_INDICES - 2,
                D3DXVECTOR3(0, 0, 0),
                D3DXVECTOR3(0, 0, 0));
            PhysicalModel * phys_mod = add_physical_model(high_cylinder_model, low_cylinder_model);

            // - Add frame --
            IndexArray frame;
            const Index LOW_VERTICES_PER_SIDE = LOW_EDGES_PER_BASE*LOW_EDGES_PER_HEIGHT;
            add_range(frame, LOW_VERTICES_PER_SIDE/4, LOW_VERTICES_PER_SIDE*3/4, LOW_EDGES_PER_BASE); // vertical line layer
            add_range(frame, LOW_VERTICES_PER_SIDE/4 + 1, LOW_VERTICES_PER_SIDE*3/4, LOW_EDGES_PER_BASE); // vertical line layer
            phys_mod->set_frame(frame);
            low_cylinder_model->repaint_vertices(frame, FRAME_COLOR);

            // - Shapes and shape callbacks definition -
            const int SHAPE_SIZE = LOW_EDGES_PER_BASE;
            const int SHAPE_LINES_OFFSET = 3;
            const int SHAPES_COUNT = 8;
            const int SHAPE_STEP = (SHAPES_COUNT > 1) ?
                                   ((LOW_EDGES_PER_HEIGHT - 2*SHAPE_LINES_OFFSET)/(SHAPES_COUNT - 1))*LOW_EDGES_PER_BASE :
                                   0;
            const int SHAPE_OFFSET = SHAPE_LINES_OFFSET*LOW_EDGES_PER_BASE;

            const int SUBSHAPES_COUNT = 4;
            const int SUBSHAPE_SIZE = SHAPE_SIZE/SUBSHAPES_COUNT;

            // let's have some static array of dynamic arrays... :)
            IndexArray vertex_indices[SHAPES_COUNT*SUBSHAPES_COUNT];
            // ...and fill it
            int subshape_index = 0;
            for(int i = 0; i < SHAPES_COUNT; ++i)
            {
                for(int j = 0; j < SUBSHAPES_COUNT; ++j)
                {
                    int subshape_start = SHAPE_OFFSET + i*SHAPE_STEP + j*SUBSHAPE_SIZE;
                    add_range(vertex_indices[subshape_index], subshape_start, subshape_start + SUBSHAPE_SIZE);
                    // create reaction
                    ShapeDeformationReaction & reaction = * new   DummyReaction( vertex_indices[subshape_index],
                                                                                 THRESHOLD_DISTANCE,
                                                                                 *low_cylinder_model );
                    reactions.push_back(&reaction);
                    // register reaction
                    phys_mod->add_shape_deformation_reaction(reaction);
                
                    ++subshape_index;
                }
            }

            IndexArray hit_point(1);
            hit_point.push_back(390); // oops, hard-coded...
        
            MessageBoxHitReaction weak_hit_reaction(hit_point, 1, _T("[All OK] Weak hit occured!"));
            MessageBoxHitReaction strong_hit_reaction(hit_point, 100, _T("[!BUG!] Strong hit occured!"));

            phys_mod->add_hit_reaction(weak_hit_reaction);
            phys_mod->add_hit_reaction(strong_hit_reaction);

            CylindricalRegion inside(Vector(0,0,cylinder_z+cylinder_height), Vector(0,0,cylinder_z), cylinder_radius - 0.1);
            CylindricalRegion outside(Vector(0,0,cylinder_z+cylinder_height), Vector(0,0,cylinder_z), cylinder_radius + 0.012);

            IndexArray shape;
            add_range(shape, 9*LOW_EDGES_PER_BASE, 10*LOW_EDGES_PER_BASE);

            MessageBoxRegionReaction inside_reaction(shape, inside, true, _T("[OK] Entered inside!"));
            MessageBoxRegionReaction outside_reaction(shape, outside, false, _T("[OK] Left out!"));

            //phys_mod->add_region_reaction(inside_reaction);
            //phys_mod->add_region_reaction(outside_reaction);

            // - Set impact -
            set_impact(Vector(0,2.2,-0.9), Vector(0, 0, 0), Vector(0,-30,0.0));
        }
    };

    class CarDemo : public Demo
    {
    public:
        CarDemo(Application & _app) : Demo(_app, 0, 0, 0, 0)
        {}
    protected:
        virtual void prepare()
        {
            // == PREPARE CAR DEMO ==

            // - Create models -
            MeshModel * car = new MeshModel(app.get_device(), lighting_shader, MESH_FILENAME, CYLINDER_COLOR, D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0, 0, 0));
            Vertex * car_vertices = car->lock_vertex_buffer();
            PointModel * low_car = new PointModel(app.get_device(), simple_shader, car_vertices, car->get_vertices_count(), 10, D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0, 0, 0));
            car->unlock_vertex_buffer();
            add_physical_model(car, low_car);

            // - Set impact -
            set_impact(Vector(0,2.2,-0.9), Vector(0, 1.15, 0), Vector(0,-110,0.0));
        }
    };

    class OvalDemo : public Demo
    {
    private:
        Model * low_oval_model;
        Model * high_ovel_model;
    public:
        OvalDemo(Application & _app)
            : Demo(_app, OVAL_VERTICES, OVAL_INDICES, OVAL_VERTICES, OVAL_INDICES)
        {}

    protected:
        virtual void prepare()
        {
            // == PREPARE OVAL DEMO ==

            // - Create models -
            const float oval_radius = 1.5;
            sphere(oval_radius, D3DXVECTOR3(0, 0, 0), CYLINDER_COLOR, OVAL_EDGES_PER_DIAMETER, low_model_vertices, low_model_indices);
            // Make oval
            squeeze_sphere(0.3f, 0, low_model_vertices, OVAL_VERTICES);
            squeeze_sphere(0.3f, 1, low_model_vertices, OVAL_VERTICES);
            // TODO: different vertices for low- and high-model (see //! below)
            Model * high_oval_model = new Model(
                app.get_device(),
                D3DPT_TRIANGLELIST,
                lighting_shader,
                low_model_vertices,  //!
                OVAL_VERTICES,       //!
                low_model_indices,   //!
                OVAL_INDICES,        //!
                OVAL_INDICES/3,
                D3DXVECTOR3(0, 0, 0),
                D3DXVECTOR3(0, 0, 0));
            Model * low_oval_model = new Model(
                app.get_device(),
                D3DPT_TRIANGLELIST,
                simple_shader,
                low_model_vertices,
                OVAL_VERTICES,
                low_model_indices,
                OVAL_INDICES,
                OVAL_INDICES/3,
                D3DXVECTOR3(0, 0, 0),
                D3DXVECTOR3(0, 0, 0));
            
            add_physical_model(high_oval_model, low_oval_model);

            // - Set impact -
            set_impact(Vector(0,2.2,0), Vector(0, 0, 0), Vector(0,-30,0.0));
        }
    };
#pragma warning( pop )
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
    
    try
    {
        Application app(logger);
        app.set_updating_vertices_on_gpu(true);

        OvalDemo demo(app);
        // or - CylinderDemo demo(app);
        // or - CarDemo demo(app);

        demo.run();
        
        logger.log("        [Renderer]", "application shutdown");
    }
    catch(RuntimeError &e)
    {
        // NOTE: now resources are released automaticaly through Demo's virtual destructor!

        logger.log("ERROR!! [Renderer]", e.get_log_entry());
        logger.dump_messages();
        const TCHAR *MESSAGE_BOX_TITLE = _T("Renderer error!");
        my_message_box(e.get_message(), MESSAGE_BOX_TITLE, MB_OK | MB_ICONERROR, true);
        return -1;
    }
    return 0;
}
