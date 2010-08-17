#include "main.h"
#include "Application.h"
#include "Model.h"
#include "cylinder.h"
#include "tessellate.h"

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
}

INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, INT )
{
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
                        D3DXVECTOR3(0.5f, 0.5f, -height/2),
                        D3DXVECTOR3(0,0,0));

        app.add_model(cylinder1);
        app.run();
        delete_array(&cylinder_indices);
        delete_array(&cylinder_vertices);
    }
    catch(RuntimeError &e)
    {
        delete_array(&cylinder_indices);
        delete_array(&cylinder_vertices);
        const TCHAR *MESSAGE_BOX_TITLE = _T("Renderer error!");
        MessageBox(NULL, e.message(), MESSAGE_BOX_TITLE, MB_OK | MB_ICONERROR);
        return -1;
    }
    return 0;
}
