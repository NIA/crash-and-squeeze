#include "main.h"
#include "Application.h"
#include "Model.h"
#include "cubic.h"
#include "plane.h"
#include "tessellate.h"
#include <fstream>
#include <ctime>
#include "Logging/logger.h"

using ::CrashAndSqueeze::Logging::logger;
using CrashAndSqueeze::Core::Force;
using CrashAndSqueeze::Core::HalfSpaceSpringForce;
using CrashAndSqueeze::Core::EverywhereForce;
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
    try
    {
        Application app;

        VertexShader simple_shader(app.get_device(), VERTEX_DECL_ARRAY, SIMPLE_SHADER_FILENAME);
        
        // -------------------------- M o d e l -----------------------

        cubic_vertices = new Vertex[CUBIC_VERTICES_COUNT];
        cubic_indices = new Index[CUBIC_INDICES_COUNT];

        cubic(0.5, 0.5, 2, D3DXVECTOR3(-0.25f, -0.25f, 3), CYLINDER_COLOR,
              cubic_vertices, cubic_indices);

        Model cube(app.get_device(),
                   D3DPT_LINELIST,
                   simple_shader,
                   sizeof(cubic_vertices[0]),
                   cubic_vertices,
                   CUBIC_VERTICES_COUNT,
                   cubic_indices,
                   CUBIC_INDICES_COUNT,
                   CUBIC_PRIMITIVES_COUNT,
                   D3DXVECTOR3(0,0,0),
                   D3DXVECTOR3(0,0,0));

        app.add_model(cube, true);
        
        // -------------------------- F o r c e s -----------------------
        const int FORCES_NUM = 3;
        const int SPRINGS_NUM = FORCES_NUM - 1;
        Force * forces[FORCES_NUM];

        HalfSpaceSpringForce springs[SPRINGS_NUM] = {
            HalfSpaceSpringForce(1200, Vector(0,0,0.25), Vector(0,0,1), 28),
            HalfSpaceSpringForce(400, Vector(0,0,4.75), Vector(0,4,-10), 28),
        };
        static EverywhereForce gravity(Vector(0, 0, -5));
        
        for(int i = 0; i < SPRINGS_NUM; ++i)
        {
            forces[i] = &springs[i];
        }
        forces[FORCES_NUM-1] = &gravity;
        app.set_forces(forces, FORCES_NUM);
        
        // ------------------- F o r c e   m o d e l s ----------------
        plane_vertices = new Vertex[PLANE_VERTICES_COUNT];
        plane_indices = new Index[PLANE_INDICES_COUNT];

        plane(5, 5, plane_vertices, plane_indices, OBSTACLE_COLOR);

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

        // -------------------------- G O ! ! ! -----------------------
        app.run();
        delete_array(&cubic_indices);
        delete_array(&cubic_vertices);
        
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
