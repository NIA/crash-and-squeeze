#include "main.h"
#include "Application.h"
#include "Model.h"
#include "cubic.h"
#include "plane.h"
#include "cylinder.h"
#include "sphere.h"
#include <ctime>

#include "Logging/logger.h"

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
    const char *SIMPLE_SHADER_FILENAME = "simple.vsh";
    const char *LIGHTING_SHADER_FILENAME = "lighting.vsh";

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

    const Index HIGH_EDGES_PER_BASE = 100; // 300
    const Index HIGH_EDGES_PER_HEIGHT = 120; // 300
    const Index HIGH_EDGES_PER_CAP = 40; // 50
    const Index HIGH_CYLINDER_VERTICES = cylinder_vertices_count(HIGH_EDGES_PER_BASE, HIGH_EDGES_PER_HEIGHT, HIGH_EDGES_PER_CAP);
    const DWORD HIGH_CYLINDER_INDICES = cylinder_indices_count(HIGH_EDGES_PER_BASE, HIGH_EDGES_PER_HEIGHT, HIGH_EDGES_PER_CAP);

    const Index SPHERE_EDGES_PER_DIAMETER = 9;
    const Index SPHERE_VERTICES = sphere_vertices_count(SPHERE_EDGES_PER_DIAMETER);
    const DWORD SPHERE_INDICES = sphere_indices_count(SPHERE_EDGES_PER_DIAMETER);
    const D3DXCOLOR HIT_REGION_COLOR = D3DCOLOR_RGBA(255, 255, 0, 128);

    inline D3DXVECTOR3 math_vector_to_d3dxvector(const Vector &v)
    {
        return D3DXVECTOR3(static_cast<float>(v[0]),
                           static_cast<float>(v[1]),
                           static_cast<float>(v[2]));
    }

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
            MessageBox(NULL, _T("Physical subsystem warning! See log"), _T("Crash-And-Squeeze warning!"), MB_OK | MB_ICONEXCLAMATION);
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
#pragma warning( default : 4512 ) 

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
            MessageBox(NULL, message, _T("Hit Reaction"), MB_OK | MB_ICONINFORMATION);
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

            MessageBox(NULL, message, _T("Region Reaction"), MB_OK | MB_ICONINFORMATION);
            this->disable();
        }
    };
}

void Logger::log(const char *prefix, const char * message, const char * file, int line)
{
    if(!log_file.is_open())
        return;

    static const int DATETIME_BUF_SIZE = 80;
    char datetime_buffer[DATETIME_BUF_SIZE];
    time_t rawtime;
    struct tm * timeinfo;
    time( &rawtime );
#pragma warning( disable : 4996 )
    timeinfo = localtime( &rawtime );
#pragma warning( default : 4996 ) 
    strftime(datetime_buffer, DATETIME_BUF_SIZE-1, "%Y-%m-%d %H:%M:%S", timeinfo);

    log_file << datetime_buffer << ' ' << prefix << ' ' << message;
    if(0 != file && '\0' != file[0])
    {
        log_file << "; " << file;
        if(0 != line)
            log_file << "(" << line << ")";
    }
    log_file << std::endl;
    log_file.flush();
}

INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, INT )
{
    Logger logger("renderer.log");
    logger.log("        [Renderer]", "application startup");
    
    PhysicsLogger & phys_logger = PhysicsLogger::get_instance();
    
    PhysLogAction phys_log_action(logger);
    PhysWarningAction phys_warn_action(logger);
    PhysErrorAction phys_err_action(logger);
    
    phys_logger.set_action(PhysicsLogger::LOG, &phys_log_action);
    phys_logger.set_action(PhysicsLogger::WARNING, &phys_warn_action);
    phys_logger.set_action(PhysicsLogger::ERROR, &phys_err_action);

    srand( static_cast<unsigned>( time(NULL) ) );
    
    Vertex * cubic_vertices = NULL;
    Index * cubic_indices = NULL;
    Vertex * low_cylinder_model_vertices = NULL;
    Index * low_cylinder_model_indices = NULL;
    Vertex * high_cylinder_model_vertices = NULL;
    Index * high_cylinder_model_indices = NULL;
    Vertex * sphere_vertices = NULL;
    Index * sphere_indices = NULL;
    
    Array<RepaintReaction*> reactions;
    try
    {
        Application app(logger);

        VertexShader simple_shader(app.get_device(), VERTEX_DECL_ARRAY, SIMPLE_SHADER_FILENAME);
        VertexShader lighting_shader(app.get_device(), VERTEX_DECL_ARRAY, LIGHTING_SHADER_FILENAME);
        
        // -------------------------- M o d e l -----------------------

        //cubic_vertices = new Vertex[CUBIC_VERTICES_COUNT];
        //cubic_indices = new Index[CUBIC_INDICES_COUNT];

        //cubic(0.5, 0.5, 2, D3DXVECTOR3(-0.25f, -0.25f, 3), CYLINDER_COLOR,
        //      cubic_vertices, cubic_indices);

        //Model cube(app.get_device(),
        //           D3DPT_LINELIST,
        //           simple_shader,
        //           cubic_vertices,
        //           CUBIC_VERTICES_COUNT,
        //           cubic_indices,
        //           CUBIC_INDICES_COUNT,
        //           CUBIC_PRIMITIVES_COUNT,
        //           D3DXVECTOR3(0,0,0),
        //           D3DXVECTOR3(0,0,0));
        //app.add_model(cube, true);
        low_cylinder_model_vertices = new Vertex[LOW_CYLINDER_VERTICES];
        low_cylinder_model_indices = new Index[LOW_CYLINDER_INDICES];
        high_cylinder_model_vertices = new Vertex[HIGH_CYLINDER_VERTICES];
        high_cylinder_model_indices = new Index[HIGH_CYLINDER_INDICES];
        
        const float cylinder_radius = 0.25;
        const float cylinder_height = 2;
        const float cylinder_z = -cylinder_height/2;

        cylinder( cylinder_radius, cylinder_height, D3DXVECTOR3(0,0,cylinder_z),
                 &CYLINDER_COLOR, 1,
                 HIGH_EDGES_PER_BASE, HIGH_EDGES_PER_HEIGHT, HIGH_EDGES_PER_CAP,
                 high_cylinder_model_vertices, high_cylinder_model_indices );
        Model high_cylinder_model(app.get_device(),
                                  D3DPT_TRIANGLESTRIP,
                                  lighting_shader,
                                  high_cylinder_model_vertices,
                                  HIGH_CYLINDER_VERTICES,
                                  high_cylinder_model_indices,
                                  HIGH_CYLINDER_INDICES,
                                  HIGH_CYLINDER_INDICES - 2,
                                  D3DXVECTOR3(0, 0, 0),
                                  D3DXVECTOR3(0, 0, 0));

        cylinder( cylinder_radius, cylinder_height, D3DXVECTOR3(0,0,cylinder_z),
                 &CYLINDER_COLOR, 1,
                 LOW_EDGES_PER_BASE, LOW_EDGES_PER_HEIGHT, LOW_EDGES_PER_CAP,
                 low_cylinder_model_vertices, low_cylinder_model_indices );
        Model low_cylinder_model(app.get_device(),
                                 D3DPT_TRIANGLESTRIP,
                                 simple_shader,
                                 low_cylinder_model_vertices,
                                 LOW_CYLINDER_VERTICES,
                                 low_cylinder_model_indices,
                                 LOW_CYLINDER_INDICES,
                                 LOW_CYLINDER_INDICES - 2,
                                 D3DXVECTOR3(0, 0, 0),
                                 D3DXVECTOR3(0, 0, 0));
        
        PhysicalModel * phys_mod = app.add_model(high_cylinder_model, true, &low_cylinder_model);
        if(NULL == phys_mod)
            throw NullPointerError();
        
        IndexArray frame;
        const Index LOW_VERTICES_PER_SIDE = LOW_EDGES_PER_BASE*LOW_EDGES_PER_HEIGHT;
        add_range(frame, LOW_VERTICES_PER_SIDE/4, LOW_VERTICES_PER_SIDE*3/4, LOW_EDGES_PER_BASE); // vertical line layer
        add_range(frame, LOW_VERTICES_PER_SIDE/4 + 1, LOW_VERTICES_PER_SIDE*3/4, LOW_EDGES_PER_BASE); // vertical line layer
        phys_mod->set_frame(frame);
        low_cylinder_model.repaint_vertices(frame, FRAME_COLOR);

        // -- shapes and shape callbacks definition --

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
                RepaintReaction & reaction = * new RepaintReaction( vertex_indices[subshape_index],
                                                                    THRESHOLD_DISTANCE,
                                                                    low_cylinder_model );
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

        phys_mod->add_region_reaction(inside_reaction);
        phys_mod->add_region_reaction(outside_reaction);

        // -------------------------- F o r c e s -----------------------
        ForcesArray forces;

        PlaneForce force( Vector(0,60,0), Vector(0,0,cylinder_z), Vector(0,0,1), 0.3 );
        forces.push_back(&force);
        app.set_forces(forces);

        SphericalRegion hit_region( Vector(0,-cylinder_radius,cylinder_z*2/3), 0.1 );
        app.set_impact( hit_region, Vector(0,45,0.0) );

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
                               math_vector_to_d3dxvector(hit_region.get_center()),
                               D3DXVECTOR3(0, 0, 0));

        app.add_model(hit_region_model, false);

        // -------------------------- G O ! ! ! -----------------------
        app.run();
        delete_array(&cubic_indices);
        delete_array(&cubic_vertices);
        delete_array(&low_cylinder_model_indices);
        delete_array(&low_cylinder_model_vertices);
        delete_array(&high_cylinder_model_indices);
        delete_array(&high_cylinder_model_vertices);
        delete_array(&sphere_indices);
        delete_array(&sphere_vertices);

        for(int i = 0; i < reactions.size(); ++i)
            delete reactions[i];
        
        logger.log("        [Renderer]", "application shutdown\n");
    }
    catch(RuntimeError &e)
    {
        delete_array(&cubic_indices);
        delete_array(&cubic_vertices);
        delete_array(&low_cylinder_model_indices);
        delete_array(&low_cylinder_model_vertices);
        delete_array(&high_cylinder_model_indices);
        delete_array(&high_cylinder_model_vertices);
        delete_array(&sphere_indices);
        delete_array(&sphere_vertices);
        for(int i = 0; i < reactions.size(); ++i)
            delete reactions[i];
        
        logger.log("ERROR!! [Renderer]", "application crash\n");
        const TCHAR *MESSAGE_BOX_TITLE = _T("Renderer error!");
        MessageBox(NULL, e.message(), MESSAGE_BOX_TITLE, MB_OK | MB_ICONERROR);
        return -1;
    }
    return 0;
}
