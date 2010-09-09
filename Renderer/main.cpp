#include "main.h"
#include "Application.h"
#include "Model.h"
#include "cubic.h"
#include "plane.h"
#include "cylinder.h"
#include <fstream>
#include <ctime>
#include "Logging/logger.h"

using ::CrashAndSqueeze::Logging::logger;
using CrashAndSqueeze::Core::Force;
using CrashAndSqueeze::Core::HalfSpaceSpringForce;
using CrashAndSqueeze::Core::EverywhereForce;
using CrashAndSqueeze::Core::CylinderSpringForce;
using CrashAndSqueeze::Math::Vector;

namespace
{
    const char *SIMPLE_SHADER_FILENAME = "simple.vsh";

    const D3DCOLOR CYLINDER_COLOR = D3DCOLOR_XRGB(150, 200, 255);
    const D3DCOLOR OBSTACLE_COLOR = D3DCOLOR_XRGB(100, 100, 100);

    std::ofstream log_file("renderer.log", std::ios::app);

    inline D3DXVECTOR3 math_vector_to_d3dxvector(const Vector &v)
    {
        return D3DXVECTOR3(static_cast<float>(v[0]),
                           static_cast<float>(v[1]),
                           static_cast<float>(v[2]));
    }

    void my_log_callback(const char * message, const char * file, int line)
    {
        my_log("        [Crash-And-Squeeze]", message, file, line);
    }

    void my_warning_callback(const char * message, const char * file, int line)
    {
        my_log("WARNING [Crash-And-Squeeze]", message, file, line);
        MessageBox(NULL, _T("Physical subsystem warning! See log"), _T("Crash-And-Squeeze warning!"), MB_OK | MB_ICONEXCLAMATION);
    }

    void my_error_callback(const char * message, const char * file, int line)
    {
        my_log("ERROR!! [Crash-And-Squeeze]", message, file, line);
        throw PhysicsError();
    }
}

void my_log(const char *prefix, const char * message, const char * file, int line)
{
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
    my_log("        [Renderer]", "application startup");
    logger.log_callback = my_log_callback;
    logger.warning_callback = my_warning_callback;
    logger.error_callback = my_error_callback;

    srand( static_cast<unsigned>( time(NULL) ) );
    
    Vertex * cubic_vertices = NULL;
    Index * cubic_indices = NULL;
    Vertex * plane_vertices = NULL;
    Index * plane_indices = NULL;
    Vertex * cylinder_vertices = NULL;
    Index * cylinder_indices = NULL;
    Vertex * cylinder_model_vertices = NULL;
    Index * cylinder_model_indices = NULL;
    try
    {
        Application app;

        VertexShader simple_shader(app.get_device(), VERTEX_DECL_ARRAY, SIMPLE_SHADER_FILENAME);
        
        // -------------------------- M o d e l -----------------------

        //cubic_vertices = new Vertex[CUBIC_VERTICES_COUNT];
        //cubic_indices = new Index[CUBIC_INDICES_COUNT];

        //cubic(0.5, 0.5, 2, D3DXVECTOR3(-0.25f, -0.25f, 3), CYLINDER_COLOR,
        //      cubic_vertices, cubic_indices);

        //Model cube(app.get_device(),
        //           D3DPT_LINELIST,
        //           simple_shader,
        //           sizeof(cubic_vertices[0]),
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
                             sizeof(cylinder_model_vertices[0]),
                             cylinder_model_vertices,
                             CYLINDER_VERTICES_COUNT,
                             cylinder_model_indices,
                             CYLINDER_INDICES_COUNT,
                             CYLINDER_INDICES_COUNT - 2,
                             D3DXVECTOR3(0, 0, 0),
                             D3DXVECTOR3(0, 0, 0));
        app.add_model(cylinder_model, true);

        
        // -------------------------- F o r c e s -----------------------
        const int FORCES_NUM = 4;
        const int SPRINGS_NUM = FORCES_NUM - 2;
        Force * forces[FORCES_NUM];

        HalfSpaceSpringForce springs[SPRINGS_NUM] = {
            HalfSpaceSpringForce(1200, Vector(0,0,0.25), Vector(0,0,1), 60),
            HalfSpaceSpringForce(400, Vector(0,0,4.5), Vector(0,1,-3), 0),
        };
        EverywhereForce gravity(Vector(0, 0, -3));
        CylinderSpringForce cylinder_force(8000, Vector(-1, 0.5, 0.5), Vector(1, 0.5, 0.5), 0.25, 150);
        
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
                     sizeof(plane_vertices[0]),
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
                        sizeof(cylinder_vertices[0]),
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
        
        if(log_file.is_open())
        {
            my_log("        [Renderer]", "application shutdown\n");
            log_file.close();
        }
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
        if(log_file.is_open())
        {
            my_log("ERROR!! [Renderer]", "application crash\n");
            log_file.close();
        }
        const TCHAR *MESSAGE_BOX_TITLE = _T("Renderer error!");
        MessageBox(NULL, e.message(), MESSAGE_BOX_TITLE, MB_OK | MB_ICONERROR);
        return -1;
    }
    return 0;
}
