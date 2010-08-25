#include "main.h"
#include "Application.h"
#include "Model.h"
#include "cylinder.h"
#include "tessellate.h"
#include <fstream>
#include <ctime>
#include "Logging/logger.h"

using ::CrashAndSqueeze::Logging::logger;

namespace
{
    const char *SIMPLE_SHADER_FILENAME = "simple.vsh";
    const D3DCOLOR colors[] =
    {
        D3DCOLOR_XRGB(250, 250, 250),
        D3DCOLOR_XRGB(250, 30, 10),
        D3DCOLOR_XRGB(250, 250, 0),
        D3DCOLOR_XRGB(30, 250, 0),
        D3DCOLOR_XRGB(0, 150, 250),
    };
    const D3DCOLOR SPHERE_COLOR = D3DCOLOR_XRGB(244, 244, 255);
    const D3DCOLOR SECOND_CYLINDER_COLOR = D3DCOLOR_XRGB(30, 250, 0);
    const unsigned colors_count = array_size(colors);

    std::ofstream log_file("renderer.log", std::ios::app);

    void my_log(const char *prefix, const char * message, const char * file = NULL, int line = 0)
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
        if(0 != file)
        {
            log_file << "; " << file;
            if(0 != line)
                log_file << "(" << line << ")";
        }
        log_file << std::endl;
        log_file.flush();
    }

    void my_log_callback(const char * message, const char * file, int line)
    {
        my_log("        [Crash-And-Squeeze]", message, file, line);
    }

    void my_warning_callback(const char * message, const char * file, int line)
    {
        my_log("WARNING [Crash-And-Squeeze]", message, file, line);
    }

    void my_error_callback(const char * message, const char * file, int line)
    {
        my_log("ERROR!! [Crash-And-Squeeze]", message, file, line);
        throw PhysicsError();
    }
}

INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, INT )
{
    my_log("        [Renderer]", "application startup");
    logger.log_callback = my_log_callback;
    logger.warning_callback = my_warning_callback;
    logger.error_callback = my_error_callback;

    srand( static_cast<unsigned>( time(NULL) ) );
    
    Vertex * cylinder_vertices = NULL;
    Index * cylinder_indices = NULL;
    try
    {
        Application app;

        VertexShader simple_shader(app.get_device(), VERTEX_DECL_ARRAY, SIMPLE_SHADER_FILENAME);
        
        // -------------------------- C y l i n d e r -----------------------

        cylinder_vertices = new Vertex[CYLINDER_VERTICES_COUNT];
        cylinder_indices = new Index[CYLINDER_INDICES_COUNT];

        float height = 2.0f;
        cylinder( 0.7f, height,
                  colors, colors_count,
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
                        D3DXVECTOR3(0.0f, 0.0f, -height/2),
                        D3DXVECTOR3(0,0,0));

        app.add_model(cylinder1);
        app.run();
        delete_array(&cylinder_indices);
        delete_array(&cylinder_vertices);
        
        if(log_file.is_open())
        {
            my_log("        [Renderer]", "application shutdown\n");
            log_file.close();
        }
    }
    catch(RuntimeError &e)
    {
        delete_array(&cylinder_indices);
        delete_array(&cylinder_vertices);
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
