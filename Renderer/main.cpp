#include "main.h"
#include "Application.h"
#include "Model.h"
#include "cubic.h"
#include "plane.h"
#include "cylinder.h"
#include <ctime>

#include "Logging/logger.h"

typedef ::CrashAndSqueeze::Logging::Logger PhysicsLogger;
using CrashAndSqueeze::Core::Force;
using CrashAndSqueeze::Core::HalfSpaceSpringForce;
using CrashAndSqueeze::Core::EverywhereForce;
using CrashAndSqueeze::Core::CylinderSpringForce;
using CrashAndSqueeze::Core::ShapeDeformationReaction;
using CrashAndSqueeze::Math::Vector;
using CrashAndSqueeze::Math::Real;
using CrashAndSqueeze::Core::IndexArray;
using CrashAndSqueeze::Collections::Array;

namespace
{
    const char *SIMPLE_SHADER_FILENAME = "simple.vsh";

    const D3DCOLOR CYLINDER_COLOR = D3DCOLOR_XRGB(100, 150, 255);
    const D3DCOLOR OBSTACLE_COLOR = D3DCOLOR_XRGB(100, 100, 100);

    const D3DXCOLOR NO_DEFORM_COLOR = CYLINDER_COLOR;//D3DCOLOR_XRGB(0, 255, 0);
    const D3DXCOLOR MAX_DEFORM_COLOR = D3DCOLOR_XRGB(255, 0, 0);

    const Real     CALLBACK_THRESHOLD = 0.0;

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

    void add_range(IndexArray &arr, int from, int to)
    {
        for(int i = from; i <= to; ++i)
        {
            arr.push_back(i);
        }
    }

    class RepaintReaction : public ShapeDeformationReaction
    {
    private:
        Model &model;

    public:
        RepaintReaction(const IndexArray &shape_vertex_indices, Real threshold, Model &model)
            : ShapeDeformationReaction(shape_vertex_indices, threshold), model(model)
        {}

        virtual void invoke(Real value)
        {
            if( 1 != get_threshold() )
            {
                D3DXCOLOR result_color;

                float alpha = static_cast<float>((value - get_threshold())/(1 - get_threshold()));
                D3DXColorLerp(&result_color, &NO_DEFORM_COLOR, &MAX_DEFORM_COLOR, alpha);
                model.repaint_vertices(get_shape_vertex_indices(), result_color);
            }
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
    Vertex * plane_vertices = NULL;
    Index * plane_indices = NULL;
    Vertex * cylinder_vertices = NULL;
    Index * cylinder_indices = NULL;
    Vertex * cylinder_model_vertices = NULL;
    Index * cylinder_model_indices = NULL;
    
    Array<RepaintReaction*> reactions;
    try
    {
        Application app(logger);

        VertexShader simple_shader(app.get_device(), VERTEX_DECL_ARRAY, SIMPLE_SHADER_FILENAME);
        
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
        cylinder_model_vertices = new Vertex[CYLINDER_VERTICES_COUNT];
        cylinder_model_indices = new Index[CYLINDER_INDICES_COUNT];
        
        cylinder( 0.25, 2, D3DXVECTOR3(-0.25f, -0.25f, 3),
                         &CYLINDER_COLOR, 1,
                         cylinder_model_vertices, cylinder_model_indices );

        Model cylinder_model(app.get_device(),
                             D3DPT_TRIANGLESTRIP,
                             simple_shader,
                             cylinder_model_vertices,
                             CYLINDER_VERTICES_COUNT,
                             cylinder_model_indices,
                             CYLINDER_INDICES_COUNT,
                             CYLINDER_INDICES_COUNT - 2,
                             D3DXVECTOR3(0, 0, 0),
                             D3DXVECTOR3(0, 0, 0));
        PhysicalModel * phys_mod = app.add_model(cylinder_model, true);
        if(NULL == phys_mod)
            throw NullPointerError();

        // -- shapes and shape callbacks definition --

        const int SHAPE_SIZE = CYLINDER_EDGES_PER_BASE;
        const int SHAPE_LINES_OFFSET = 3;
        const int SHAPES_COUNT = 8;
        const int SHAPE_STEP = (SHAPES_COUNT > 1) ?
                               ((CYLINDER_EDGES_PER_HEIGHT - 2*SHAPE_LINES_OFFSET)/(SHAPES_COUNT - 1))*CYLINDER_EDGES_PER_BASE :
                               0;
        const int SHAPE_OFFSET = SHAPE_LINES_OFFSET*CYLINDER_EDGES_PER_BASE;

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
                add_range(vertex_indices[subshape_index], subshape_start, subshape_start + SUBSHAPE_SIZE - 1);
                // register reaction
                RepaintReaction & reaction = * new RepaintReaction( vertex_indices[subshape_index],
                                                                    CALLBACK_THRESHOLD,
                                                                    cylinder_model );
                reactions.push_back(&reaction);
                phys_mod->add_shape_deformation_reaction(reaction);
                // do initial repaint
                reaction.invoke(CALLBACK_THRESHOLD);
                
                ++subshape_index;
            }
        }

        // -------------------------- F o r c e s -----------------------
        const int FORCES_NUM = 4;
        const int SPRINGS_NUM = FORCES_NUM - 2;
        Force * forces[FORCES_NUM];

        HalfSpaceSpringForce springs[SPRINGS_NUM] = {
            HalfSpaceSpringForce(8000, Vector(0,0,0.25), Vector(0,0,1), 100),
            HalfSpaceSpringForce(400, Vector(0,0,4.5), Vector(0,1,-3), 0),
        };
        EverywhereForce gravity(Vector(0, 0, -3));
        CylinderSpringForce cylinder_force(15000, Vector(-1, 0.5, 0.5), Vector(1, 0.5, 0.5), 0.25, 150);
        
        for(int i = 0; i < SPRINGS_NUM; ++i)
        {
            forces[i] = &springs[i];
        }
        forces[SPRINGS_NUM] = &gravity;
        forces[SPRINGS_NUM+1] = &cylinder_force;
        app.set_forces(forces, FORCES_NUM);
        
        // ------------------- F o r c e   m o d e l s ----------------
        // ---- 1: Plane
        plane_vertices = new Vertex[PLANE_VERTICES_COUNT];
        plane_indices = new Index[PLANE_INDICES_COUNT];

        plane(7, 7, plane_vertices, plane_indices, OBSTACLE_COLOR);

        Model plane1(app.get_device(),
                     D3DPT_TRIANGLELIST,
                     simple_shader,
                     plane_vertices,
                     PLANE_VERTICES_COUNT,
                     plane_indices,
                     PLANE_INDICES_COUNT,
                     PLANE_INDICES_COUNT/VERTICES_PER_TRIANGLE,
                     math_vector_to_d3dxvector(springs[0].get_plane_point()),
                     D3DXVECTOR3(0,0,0));
        app.add_model(plane1, false);
        
        // ---- 2: Cylinder
        cylinder_vertices = new Vertex[CYLINDER_VERTICES_COUNT];
        cylinder_indices = new Index[CYLINDER_INDICES_COUNT];
        
        float radius = static_cast<float>( cylinder_force.get_radius() );
        float height = static_cast<float>( distance(cylinder_force.get_point1(), cylinder_force.get_point2()) );
        cylinder( radius, height, D3DXVECTOR3(0, 0, 0),
                         &OBSTACLE_COLOR, 1,
                         cylinder_vertices, cylinder_indices );

        Model cylinder1(app.get_device(),
                        D3DPT_TRIANGLESTRIP,
                        simple_shader,
                        cylinder_vertices,
                        CYLINDER_VERTICES_COUNT,
                        cylinder_indices,
                        CYLINDER_INDICES_COUNT,
                        CYLINDER_INDICES_COUNT - 2,
                        math_vector_to_d3dxvector(cylinder_force.get_point1()),
                        D3DXVECTOR3(0, D3DX_PI/2, 0));
        app.add_model(cylinder1, false);

        // -------------------------- G O ! ! ! -----------------------
        app.run();
        delete_array(&cubic_indices);
        delete_array(&cubic_vertices);
        delete_array(&plane_indices);
        delete_array(&plane_vertices);
        delete_array(&cylinder_indices);
        delete_array(&cylinder_vertices);
        delete_array(&cylinder_model_indices);
        delete_array(&cylinder_model_vertices);

        for(int i = 0; i < reactions.size(); ++i)
            delete reactions[i];
        
        logger.log("        [Renderer]", "application shutdown\n");
    }
    catch(RuntimeError &e)
    {
        delete_array(&cubic_indices);
        delete_array(&cubic_vertices);
        delete_array(&plane_indices);
        delete_array(&plane_vertices);
        delete_array(&cylinder_indices);
        delete_array(&cylinder_vertices);
        delete_array(&cylinder_model_indices);
        delete_array(&cylinder_model_vertices);
        for(int i = 0; i < reactions.size(); ++i)
            delete reactions[i];
        
        logger.log("ERROR!! [Renderer]", "application crash\n");
        const TCHAR *MESSAGE_BOX_TITLE = _T("Renderer error!");
        MessageBox(NULL, e.message(), MESSAGE_BOX_TITLE, MB_OK | MB_ICONERROR);
        return -1;
    }
    return 0;
}
