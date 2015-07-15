#include "main.h"
#include "Application.h"
#include "Model.h"
#include "cubic.h"
#include "plane.h"
#include "cylinder.h"
#include "sphere.h"
#include "ObjMeshLoader.h"
#include "Stopwatch.h"
#include <ctime>
#if defined UNICODE || defined _UNICODE
#include <codecvt>
#endif // defined UNICODE || defined _UNICODE
#include <vector>
#include <sstream>
#include "logger.h" // application logger

#include "Logging/logger.h" // crash-and-squeeze logger

typedef ::CrashAndSqueeze::Logging::Logger PhysicsLogger;
using CrashAndSqueeze::Core::ForcesArray;
using CrashAndSqueeze::Core::PlaneForce;
using CrashAndSqueeze::Core::SphericalRegion;
using CrashAndSqueeze::Core::IRegion;
using CrashAndSqueeze::Core::CylindricalRegion;
using CrashAndSqueeze::Core::BoxRegion;
using CrashAndSqueeze::Core::Reaction;
using CrashAndSqueeze::Core::ShapeDeformationReaction;
using CrashAndSqueeze::Core::HitReaction;
using CrashAndSqueeze::Core::RegionReaction;
using CrashAndSqueeze::Math::Vector;
using CrashAndSqueeze::Math::VECTOR_SIZE;
using CrashAndSqueeze::Math::Real;
using CrashAndSqueeze::Core::IndexArray;
using CrashAndSqueeze::Collections::Array;

// TODO: move this physics out of main somewhere else?
#include "Core/discrete_vector_field.h"
using CrashAndSqueeze::Math::Point;
using CrashAndSqueeze::Math::ISpace;
using CrashAndSqueeze::Math::SphericalCoords;
static const Real PI = CrashAndSqueeze::Math::SphericalCoords::PI;
using CrashAndSqueeze::Math::UnitSphere2D;
using CrashAndSqueeze::Core::DiscreteVectorField;

using DirectX::XMVECTOR;
using DirectX::XMStoreFloat4;
using DirectX::XMStoreFloat4;
using DirectX::XMVectorLerp;

using std::string;
using std::ostringstream;

// TODO: store these options in config file instead of source code
namespace
{
    bool DISABLE_MESSAGE_BOXES = true;
    bool PAINT_MODEL = false;
    bool SHOW_NORMALS = false;
    bool ENABLE_REACTIONS = false;
    bool SHOW_REACTION_REGIONS = false;
    bool UPDATE_ON_GPU = true; // TODO: when update on GPU, create models as immutable (dynamic = false)
                               // TODO: why is lighting less smooth when updating on GPU? Maybe precision of applying deformation and averaging? How to fix?

    const char *SIMPLE_SHADER_FILENAME = "simple.vsh";
                                                         // TODO: can we handle this with one shader file? And allow switching CPU/GPU in runtime?
    const char *DEFORM_SHADER_FILENAME = UPDATE_ON_GPU ? (CAS_QUADRATIC_EXTENSIONS_ENABLED ? "deform_qx.vsh" : "deform.vsh") : "simple.vsh";
    const char *LIGHTING_SHADER_FILENAME = "lighting.psh";
    const char *SIMPLE_PIXEL_SHADER_FILENAME = "simple.psh";

    const TCHAR *DEFAULT_MESH_FILENAME = _T("heart.obj");
    const float4 MESH_COLOR(0.9f, 0.3f, 0.3f, 1);
    bool  MESH_AUTOSCALE = true;
    const float MESH_EXPECTED_DIMENSION = 7.5f; // actually an arbitrary number, but reaction regions and camera/hit position depend on it not obviously...

    const float4 CYLINDER_COLOR (0.4f, 0.6f, 1.0f, 1);
    const float4 OBSTACLE_COLOR (0.4f, 0.4f, 0.4f, 1);

    const float4 NO_DEFORM_COLOR = CYLINDER_COLOR;
    const float4 MAX_DEFORM_COLOR (1, 0, 0, 1);

    const float4 FRAME_COLOR (0.5f, 1, 0, 1);

    const Real      THRESHOLD_DISTANCE = 0.05;

    const Index LOW_EDGES_PER_BASE = 60;
    const Index LOW_EDGES_PER_HEIGHT = 68;
    const Index LOW_EDGES_PER_CAP = 8;
    const Index LOW_CYLINDER_VERTICES = cylinder_vertices_count(LOW_EDGES_PER_BASE, LOW_EDGES_PER_HEIGHT, LOW_EDGES_PER_CAP);
    const Index LOW_CYLINDER_INDICES = cylinder_indices_count(LOW_EDGES_PER_BASE, LOW_EDGES_PER_HEIGHT, LOW_EDGES_PER_CAP);

    const Index HIGH_EDGES_PER_BASE = 100; // 300; //
    const Index HIGH_EDGES_PER_HEIGHT = 120; // 300; //
    const Index HIGH_EDGES_PER_CAP = 40; // 50; //
    const Index HIGH_CYLINDER_VERTICES = cylinder_vertices_count(HIGH_EDGES_PER_BASE, HIGH_EDGES_PER_HEIGHT, HIGH_EDGES_PER_CAP);
    const Index HIGH_CYLINDER_INDICES = cylinder_indices_count(HIGH_EDGES_PER_BASE, HIGH_EDGES_PER_HEIGHT, HIGH_EDGES_PER_CAP);

    const Index SPHERE_EDGES_PER_DIAMETER = 8;
    const Index SPHERE_VERTICES = sphere_vertices_count(SPHERE_EDGES_PER_DIAMETER);
    const Index SPHERE_INDICES = sphere_indices_count(SPHERE_EDGES_PER_DIAMETER);
    const float4 HIT_REGION_COLOR (1, 1, 0, 0.5f);

    const Index LOW_OVAL_EDGES_PER_DIAMETER = 70;
    const Index LOW_OVAL_VERTICES = sphere_vertices_count(LOW_OVAL_EDGES_PER_DIAMETER);
    const DWORD LOW_OVAL_INDICES = sphere_indices_count(LOW_OVAL_EDGES_PER_DIAMETER);

    const Index HIGH_OVAL_EDGES_PER_DIAMETER = 220;
    const Index HIGH_OVAL_VERTICES = sphere_vertices_count(HIGH_OVAL_EDGES_PER_DIAMETER);
    const DWORD HIGH_OVAL_INDICES = sphere_indices_count(HIGH_OVAL_EDGES_PER_DIAMETER);

    const Index GLOBE_EDGES_PER_DIAMETER = 50;
    const Index GLOBE_VERTICES = sphere_vertices_count(GLOBE_EDGES_PER_DIAMETER);
    const Index GLOBE_INDICES  = sphere_indices_count(GLOBE_EDGES_PER_DIAMETER);
    const Index GLOBE_CURVE_VERTICES = 37;

    inline std::ostream &operator<<(std::ostream &stream, const Vector &vector)
    {
        return stream << "(" << vector[0] << ", " << vector[1] << ", " << vector[2] << ")";
    }

    class StringFormat {
    public:
        template<class T>
        StringFormat& operator<< (const T& arg) {
            stream << arg;
            return *this;
        }
        operator std::string() const {
            return stream.str();
        }
    protected:
        std::ostringstream stream;
    };

    inline void my_message_box(const TCHAR *message, const TCHAR *caption, UINT type, bool force = false)
    {
        if( ! DISABLE_MESSAGE_BOXES || force )
        {
            MessageBox(NULL, message, caption, type);
        }
    }

    inline void override_app_settings(Application &app, bool paint_model, bool show_normals, bool update_on_gpu)
    {
        SimulationSettings sim;
        GlobalSettings glo;
        RenderSettings ren;
        app.get_settings(sim, glo, ren);
        glo.update_vertices_on_gpu = update_on_gpu;
        ren.show_normals = show_normals;
        ren.paint_clusters = paint_model;
        app.set_settings(sim, glo, ren);
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
            Vertex * buffer = gfx_model.lock_vertex_buffer(LOCK_READ);
            int gfx_vertices_count = static_cast<int>(gfx_model.get_vertices_count());
            for(int i = 0; i < gfx_vertices_count; ++i)
            {
                if( region.contains( float3_to_math_vector(buffer[i].pos) ) )
                    graphical_vertices.push_back(i);
            }
            gfx_model.unlock_vertex_buffer();
        }

        IRegion & get_region() { return region; }
        IndexArray & get_physical_vertices() { return physical_vertices; }
        IndexArray & get_graphical_vertices() { return graphical_vertices; }
    };

    // Helper to allow RegionWithVertices initialization before calling base constructor of RepaintReaction (http://stackoverflow.com/a/5898066/693538)
    struct RegionWithVerticesHelper
    {
        RegionWithVertices rgnwv;

        RegionWithVerticesHelper(IRegion &region, PhysicalModel &phy_mod, AbstractModel &gfx_mod)
            : rgnwv(region, phy_mod, gfx_mod)
        {}
    };

    class RepaintReaction : private RegionWithVerticesHelper, public ShapeDeformationReaction
    {
    private:
        Model &model;
        Real max_distance;

        float4 no_deform_color;
        float4 max_deform_color;

    public:
        RepaintReaction(IRegion &region,
                        PhysicalModel &phy_mod,
                        Model &gfx_mod,
                        Real threshold_distance,
                        Real max_distance,
                        float4 no_deform_color = NO_DEFORM_COLOR,
                        float4 max_deform_color = MAX_DEFORM_COLOR)
            : RegionWithVerticesHelper(region, phy_mod, gfx_mod),
              ShapeDeformationReaction(rgnwv.get_physical_vertices(), threshold_distance),
              model(gfx_mod), max_distance(max_distance),
              no_deform_color(no_deform_color), max_deform_color(max_deform_color)
        {
            // TEST REGION!
            //model.repaint_vertices(rgnwv.get_graphical_vertices(), float4(1,1,0,1));
        }

        virtual void invoke(int vertex_index, Real distance)
        {
            UNREFERENCED_PARAMETER(vertex_index);

            float4 color;
            Real coeff = (distance - get_threshold_distance())/(max_distance - get_threshold_distance());
            if(coeff > 1)
                coeff = 1;

            XMStoreFloat4(&color, XMVectorLerp(XMLoadFloat4(&no_deform_color), XMLoadFloat4(&max_deform_color), static_cast<float>(coeff)));
            model.repaint_vertices(rgnwv.get_graphical_vertices(), color);
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

    Model * box_to_model(IRenderer * renderer, VertexShader & vertex_shader, PixelShader & pixel_shader, const BoxRegion * box)
    {
        SimpleCube box_geom(math_vector_to_float3(box->get_min_corner()), math_vector_to_float3(box->get_max_corner()), random_color(0.6f));
        Model * box_model = new Model(
            renderer,
            box_geom.get_topology(),
            vertex_shader,
            box_geom.get_vertices(),
            box_geom.get_vertices_num(),
            box_geom.get_indices(),
            box_geom.get_indices_num(),
            false);
        box_model->set_draw_ccw(true);
        box_model->add_shader(pixel_shader);
        return box_model;
    }

    void paint_model(AbstractModel &model)
    {
        static const int COLORS_COUNT = 5;
        static float4 colors[COLORS_COUNT] =
        {
            float4(0.0f, 0.0f, 1.0f, 1),
            float4(0.0f, 0.6f, 0.0f, 1),
            float4(0.6f, 1.0f, 0.0f, 1),
            float4(1.0f, 1.0f, 0.0f, 1),
            float4(0.0f, 1.0f, 1.0f, 1),
        };
        Vertex * vertices = model.lock_vertex_buffer(LOCK_READ_WRITE);
        for(unsigned i = 0; i < model.get_vertices_count(); ++i)
        {
            int color_index = vertices[i].clusters_num % COLORS_COUNT;
            vertices[i].color = colors[color_index];
        }
        model.unlock_vertex_buffer();
    };

    class BoxReactionsGenerator
    {
    private:
        std::vector<Reaction*> reactions;
        std::vector<IRegion*> regions;
    public:
        class IReactionFactory
        {
        public:
            virtual Reaction * create_reaction(BoxRegion * box) = 0;
        };

        void generate(Vector box_min, Vector box_max, int (&cells_nums)[VECTOR_SIZE], IReactionFactory * reaction_factory)
        {
            if (!ENABLE_REACTIONS)
                return;

            Vector cell_steps[VECTOR_SIZE] = {Vector(1,0,0), Vector(0,1,0), Vector(0,0,1) };

            // Initialize cell_steps and box_end
            Vector box_end = box_min;
            for (int i = 0; i < VECTOR_SIZE; ++i)
            {
                cell_steps[i] *= (box_max[i] - box_min[i])/cells_nums[i];
                box_end += cell_steps[i]; // it was like box_end = box_min + x_step + y_step + z_step;
            }
            for(int ix = 0; ix < cells_nums[0]; ++ix)
            {
                for(int iy = 0; iy < cells_nums[1]; ++iy)
                {
                    for(int iz = 0; iz < cells_nums[2]; ++iz)
                    {
                        Vector offset = ix*cell_steps[0] + iy*cell_steps[1] + iz*cell_steps[2];
                        BoxRegion * box = new BoxRegion(box_min + offset, box_end + offset);

                        Reaction * reaction = reaction_factory->create_reaction(box);
                        regions.push_back(box);
                        reactions.push_back(reaction);
                    }
                }
            }

        }
        ~BoxReactionsGenerator()
        {
            for (auto r: reactions)
                delete r;
            for (auto r: regions)
                delete r;
        }
    };

    class Demo : public BoxReactionsGenerator::IReactionFactory
    {
    protected:
        Vertex * low_model_vertices;
        Index * low_model_indices;
        Vertex * high_model_vertices;
        Index * high_model_indices;

        Application & app;
        VertexShader simple_shader;
        VertexShader deform_shader;
        PixelShader lighting_shader;
        PixelShader simple_pixel_shader;

        SphericalRegion * hit_region;
        std::vector<AbstractModel *> models;

        ForcesArray NO_FORCES;

        BoxReactionsGenerator reactions_generator;
        // Params needed to implement IReactionFactory which is passed to BoxReactionsGenerator
        struct RepaintReactionParams
        {
            Model * model_to_repaint;
            PhysicalModel * phys_mod;
            Real threshold_dist;
            Real max_dist;
            float4 initial_color;

            RepaintReactionParams() : model_to_repaint(nullptr), phys_mod(nullptr), threshold_dist(DBL_MAX), max_dist(DBL_MAX), initial_color(NO_DEFORM_COLOR) {}
            bool are_set() { return nullptr != model_to_repaint && nullptr != phys_mod; }
        } react_params;

        // Override this to define Demo models, reactions, etc...
        virtual void prepare() = 0;

        PhysicalModel * add_physical_model(AbstractModel * high_model, AbstractModel *low_model)
        {
            models.push_back(high_model);
            models.push_back(low_model);
            PhysicalModel * phys_mod = app.add_model(*high_model, true, low_model);
            if(NULL == phys_mod)
                throw NullPointerError(RT_ERR_ARGS("(Demo) Failed to add physical model!"));
            if(PAINT_MODEL) {
                paint_model(*high_model);
                paint_model(*low_model);
            }
            if(SHOW_NORMALS) {
                add_normals_model_for(high_model);
            }
            return phys_mod;
        }

        void add_auxiliary_model(AbstractModel * model)
        {
            models.push_back(model);
            app.add_model(*model, false);
        }

        void add_normals_model_for(AbstractModel * parent_model, float normal_length = 0.1f, bool normalize_before_showing = true)
        {
            NormalsModel* normals_model = new NormalsModel(parent_model, simple_shader, normal_length, normalize_before_showing);
            normals_model->add_shader(simple_pixel_shader);
            parent_model->add_subscriber(normals_model);
            add_auxiliary_model(normals_model);
        }

        void set_impact(Vector hit_position, double hit_radius, Vector hit_rotation_center, Vector hit_velocity)
        {
            hit_region = new SphericalRegion( hit_position, hit_radius );

            // ------------------ V i s u a l i z a t i o n -----------------------
            Vertex * sphere_vertices = new Vertex[SPHERE_VERTICES];
            Index * sphere_indices = new Index[SPHERE_INDICES];

            sphere(static_cast<float>(hit_region->get_radius()), float3(0,0,0), HIT_REGION_COLOR,
                   SPHERE_EDGES_PER_DIAMETER, sphere_vertices, sphere_indices);

            Model * hit_region_model = new Model(
                app.get_renderer(),
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
                simple_shader,
                sphere_vertices,
                SPHERE_VERTICES,
                sphere_indices,
                SPHERE_INDICES,
                false,
                math_vector_to_float3(hit_region->get_center()));
            models.push_back(hit_region_model);
            hit_region_model->add_shader(simple_pixel_shader);
            hit_region_model->set_draw_ccw(true);

            app.set_impact( *hit_region, hit_velocity, hit_rotation_center, *hit_region_model);

            delete_array(sphere_vertices);
            delete_array(sphere_indices);
        }

        void set_repaint_reaction_params(Model * model_to_repaint, PhysicalModel * phys_mod, Real threshold_dist = 0.1, Real max_dist = 0.3, float4 initial_color = NO_DEFORM_COLOR)
        {
            react_params.model_to_repaint = model_to_repaint;
            react_params.phys_mod = phys_mod;
            react_params.threshold_dist = threshold_dist;
            react_params.max_dist = max_dist;
            react_params.initial_color = initial_color;
        }

        void set_camera_position(float rho, float theta, float phi) { app.set_camera_position(rho, theta, phi); }

    public:
        Demo(Application & _app, Index low_vertices_count, DWORD low_indices_count, Index high_vertices_count, DWORD high_indices_count)
            : app(_app),
              simple_shader(_app.get_renderer(), VERTEX_DESC, VERTEX_DESC_NUM, SIMPLE_SHADER_FILENAME),
              deform_shader(_app.get_renderer(), VERTEX_DESC, VERTEX_DESC_NUM, DEFORM_SHADER_FILENAME),
              lighting_shader(_app.get_renderer(), LIGHTING_SHADER_FILENAME),
              simple_pixel_shader(_app.get_renderer(), SIMPLE_PIXEL_SHADER_FILENAME),
              hit_region(NULL)
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

        // Implement BoxReactionsGenerator::IReactionFactory
        virtual Reaction * create_reaction(BoxRegion * box) override
        {
            if ( ! react_params.are_set() )
                throw NullPointerError(RT_ERR_ARGS("(Demo) repaint reaction params was not set: call set_repaint_reaction_params first"));
            RepaintReaction * reaction =
                new RepaintReaction(*box, *react_params.phys_mod, *react_params.model_to_repaint, react_params.threshold_dist, react_params.max_dist, react_params.initial_color);
            react_params.phys_mod->add_shape_deformation_reaction(*reaction);

            if (SHOW_REACTION_REGIONS)
            {
                // visualize the above box
                add_auxiliary_model( box_to_model(app.get_renderer(), simple_shader, simple_pixel_shader, box) );
            }
            return reaction;
        }

        virtual ~Demo()
        {
            delete_array(low_model_indices);
            delete_array(low_model_vertices);
            delete_array(high_model_indices);
            delete_array(high_model_vertices);
            for (auto model: models)
                delete model;
            delete hit_region;
        }
    };

    class CylinderDemo : public Demo
    {
    public:
        CylinderDemo(Application & _app, float cylinder_radius, float cylinder_height)
            : Demo(_app, LOW_CYLINDER_VERTICES, LOW_CYLINDER_INDICES, HIGH_CYLINDER_VERTICES, HIGH_CYLINDER_INDICES),
              cylinder_radius(cylinder_radius), cylinder_height(cylinder_height), cylinder_z(-cylinder_height/2),
              hit_point(1),
              inside(Vector(0,0,cylinder_z+cylinder_height), Vector(0,0,cylinder_z), cylinder_radius - 0.1),
              outside(Vector(0,0,cylinder_z+cylinder_height), Vector(0,0,cylinder_z), cylinder_radius + 0.012)
        {}

        virtual ~CylinderDemo()
        {
            for(int i = 0; i < reactions.size(); ++i)
                delete reactions[i];
        }

        static const TCHAR * cmdline_option;

    private:
        const float cylinder_radius;
        const float cylinder_height;
        const float cylinder_z;

        // All reactions in one array (to easily delete them)
        Array<Reaction*> reactions;

        // For Shapes and shape callbacks
        static const int SHAPE_SIZE = LOW_EDGES_PER_BASE;
        static const int SHAPE_LINES_OFFSET = 3;
        static const int SHAPES_COUNT = 8;
        static const int SHAPE_STEP = (SHAPES_COUNT > 1) ?
            ((LOW_EDGES_PER_HEIGHT - 2*SHAPE_LINES_OFFSET)/(SHAPES_COUNT - 1))*LOW_EDGES_PER_BASE :
            0;
        static const int SHAPE_OFFSET = SHAPE_LINES_OFFSET*LOW_EDGES_PER_BASE;
        static const int SUBSHAPES_COUNT = 4;
        static const int SUBSHAPE_SIZE = SHAPE_SIZE/SUBSHAPES_COUNT;
        IndexArray vertex_indices[SHAPES_COUNT*SUBSHAPES_COUNT];

        // For hit reaction
        IndexArray hit_point;

        // For region reaction
        CylindricalRegion inside;
        CylindricalRegion outside;
        IndexArray shape;

    protected:
        virtual void prepare()
        {
            // == PREPARE CYLINDER DEMO ==

            // - Create models -
            cylinder( cylinder_radius, cylinder_height, float3(0,0,cylinder_z),
                     &CYLINDER_COLOR, 1,
                     HIGH_EDGES_PER_BASE, HIGH_EDGES_PER_HEIGHT, HIGH_EDGES_PER_CAP,
                     high_model_vertices, high_model_indices );
            Model * high_cylinder_model = new Model(
                app.get_renderer(),
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
                deform_shader,
                high_model_vertices,
                HIGH_CYLINDER_VERTICES,
                high_model_indices,
                HIGH_CYLINDER_INDICES);
            high_cylinder_model->add_shader(lighting_shader); // add lighting
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            float random_amp = 0.3f;
#else
            float random_amp = 0.0f; // no need for randomization without quadratic extensions
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
            cylinder( cylinder_radius, cylinder_height, float3(0,0,cylinder_z),
                     &CYLINDER_COLOR, 1,
                     LOW_EDGES_PER_BASE, LOW_EDGES_PER_HEIGHT, LOW_EDGES_PER_CAP,
                     low_model_vertices, low_model_indices, random_amp );
            Model * low_cylinder_model = new Model(
                app.get_renderer(),
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
                simple_shader,
                low_model_vertices,
                LOW_CYLINDER_VERTICES,
                low_model_indices,
                LOW_CYLINDER_INDICES);
            low_cylinder_model->add_shader(simple_pixel_shader);
            PhysicalModel * phys_mod = add_physical_model(high_cylinder_model, low_cylinder_model);

            // - Set impact -
            set_impact(Vector(0, 0.64, 0), 0.25, Vector(0, 0, 0), Vector(0,-70,0.0));

            // - Add frame --
            IndexArray frame;
            const Index LOW_VERTICES_PER_SIDE = LOW_EDGES_PER_BASE*LOW_EDGES_PER_HEIGHT;
            add_range(frame, LOW_VERTICES_PER_SIDE/4, LOW_VERTICES_PER_SIDE*3/4, LOW_EDGES_PER_BASE); // vertical line layer
            add_range(frame, LOW_VERTICES_PER_SIDE/4 + 1, LOW_VERTICES_PER_SIDE*3/4, LOW_EDGES_PER_BASE); // vertical line layer
            phys_mod->set_frame(frame);
            low_cylinder_model->repaint_vertices(frame, FRAME_COLOR);

            // - Shapes and shape callbacks definition -
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

            // -- Hit reactions --

            hit_point.push_back(390); // oops, hard-coded...

            HitReaction * weak_hit_reaction =   new MessageBoxHitReaction(hit_point, 1,   _T("[All OK] Weak hit occured!"));
            HitReaction * strong_hit_reaction = new MessageBoxHitReaction(hit_point, 100, _T("[!BUG!] Strong hit occured!"));

            phys_mod->add_hit_reaction(*weak_hit_reaction);
            phys_mod->add_hit_reaction(*strong_hit_reaction);
            reactions.push_back(weak_hit_reaction);
            reactions.push_back(strong_hit_reaction);

            // -- Region reactions --

            add_range(shape, 9*LOW_EDGES_PER_BASE, 10*LOW_EDGES_PER_BASE);

            RegionReaction * inside_reaction = new MessageBoxRegionReaction(shape, inside, true, _T("[OK] Entered inside!"));
            RegionReaction * outside_reaction = new MessageBoxRegionReaction(shape, outside, false, _T("[OK] Left out!"));
            reactions.push_back(inside_reaction);
            reactions.push_back(outside_reaction);

            phys_mod->add_region_reaction(*inside_reaction);
            phys_mod->add_region_reaction(*outside_reaction);
        }
    };
    const TCHAR * CylinderDemo::cmdline_option = _T("/cylinder");

#if defined UNICODE || defined _UNICODE
    inline string tstring_to_string(const tstring &wstr)
    {
        // based on http://stackoverflow.com/a/18374698/693538
        typedef std::codecvt_utf8<wchar_t> convert_type;
        std::wstring_convert<convert_type, wchar_t> converter;
        return converter.to_bytes(wstr);
    }
#else // defined UNICODE || defined _UNICODE
    inline const string & tstring_to_string(const tstring &str) { return str; }
#endif // defined UNICODE || defined _UNICODE

    class MeshDemo : public Demo
    {
    private:
        const tstring mesh_filename;
    public:
        MeshDemo(Application & app_, const tstring & mesh_filename_ = DEFAULT_MESH_FILENAME)
            : Demo(app_, 0, 0, 0, 0), mesh_filename(mesh_filename_)
        {}
    protected:
        virtual void prepare()
        {
            // == PREPARE MESH DEMO ==

            // - Load model from obj
            ObjMeshLoader loader(mesh_filename.c_str(), MESH_COLOR);
            {
                Stopwatch stopwatch;
                stopwatch.start();

                // load
                loader.load();

                if (MESH_AUTOSCALE)
                {
                    // scale so that dimensions are approximately MESH_EXPECTED_DIMENSION
                    float3 dimensions = loader.get_dimensions();
                    float max_dimension = std::max(std::max(dimensions.x, dimensions.y), dimensions.z);
                    if ( fabs(max_dimension) < 1e-5 ) // TODO: normal FP comparison
                        throw OutOfRangeError(RT_ERR_ARGS("Mesh has zero dimension, cannot scale it"));
                    loader.scale(MESH_EXPECTED_DIMENSION / max_dimension);
                }

                double time = stopwatch.stop();
                static const int BUF_SIZE = 128;
                static char buf[BUF_SIZE];
                sprintf_s(buf, BUF_SIZE,
                    "loading mesh from %s: %7.2f ms",
                    tstring_to_string(mesh_filename).c_str(), time*1000);
                app.get_logger().log("        [Importer]", buf);
            }

            // - Create models -
            Model * mesh = new Model(
                app.get_renderer(),
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
                deform_shader,
                // TODO: introduce IGeometry interface for easily passing ObjMeshLoader (and similar classes, e.g. SimpleCube) to Model constructor
                loader.get_vertices().data(),
                loader.get_vertices().size(),
                loader.get_indices().data(),
                loader.get_indices().size()
            );
            mesh->add_shader(lighting_shader); // add lighting
            Vertex * car_vertices = mesh->lock_vertex_buffer(LOCK_READ);
            PointModel * low_mesh = new PointModel(app.get_renderer(), simple_shader, car_vertices, mesh->get_vertices_count(), 10);
            mesh->unlock_vertex_buffer();
            low_mesh->add_shader(simple_pixel_shader);
            set_camera_position(6.1f, 1.2f, -0.1575f);
            PhysicalModel *phys_mod = add_physical_model(mesh, low_mesh);

            // - Set impact -
            set_impact(Vector(0.7,-1.5,2.7), 0.45, phys_mod->get_center_of_mass(), Vector(0,0,-510));
            app.set_impact_rot_axis(1);

            // - Reactions -
            int xyz_cells[3] = {6, 6, 1};
            set_repaint_reaction_params(mesh, phys_mod, 0.1, 0.5, MESH_COLOR);
            reactions_generator.generate(Vector(-1, -3.5, 1), Vector(2.5, 0.5, 3), xyz_cells, this);
        }
    };

    class OvalDemo : public Demo
    {
    public:
        OvalDemo(Application & _app)
            : Demo(_app, LOW_OVAL_VERTICES, LOW_OVAL_INDICES, HIGH_OVAL_VERTICES, HIGH_OVAL_INDICES)
        {}

        static const TCHAR * cmdline_option;
        static const float4 OVAL_COLOR;

    private:
        static void make_oval(Vertex * vertices, Index vertices_count)
        {
            squeeze_sphere(0.75f, 1, vertices, vertices_count);
            squeeze_sphere(0.5f, 0,  vertices, vertices_count);
        }

    protected:
        virtual void prepare()
        {
            // == PREPARE OVAL DEMO ==

            // - Create models -
            const float oval_radius = 2;
            sphere(oval_radius, float3(0, 0, 0), OVAL_COLOR, LOW_OVAL_EDGES_PER_DIAMETER, low_model_vertices, low_model_indices);
            sphere(oval_radius, float3(0, 0, 0), OVAL_COLOR, HIGH_OVAL_EDGES_PER_DIAMETER, high_model_vertices, high_model_indices);
            // Make oval
            make_oval(low_model_vertices,  LOW_OVAL_VERTICES);
            make_oval(high_model_vertices, HIGH_OVAL_VERTICES);
            Model * high_oval_model = new Model(
                app.get_renderer(),
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
                deform_shader,
                high_model_vertices,
                HIGH_OVAL_VERTICES,
                high_model_indices,
                HIGH_OVAL_INDICES
            );
            high_oval_model->add_shader(lighting_shader); // add lighting
            Model * low_oval_model = new Model(
                app.get_renderer(),
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
                simple_shader,
                low_model_vertices,
                LOW_OVAL_VERTICES,
                low_model_indices,
                LOW_OVAL_INDICES);
            set_camera_position(3.1f, 0.9f, -0.854f);
            low_oval_model->add_shader(simple_pixel_shader);

            PhysicalModel * phys_mod = add_physical_model(high_oval_model, low_oval_model);

            // - Reactions -
            int xyz_cells[3] = {6, 1, 8};
            set_repaint_reaction_params(high_oval_model, phys_mod, 0.1, 0.3, OVAL_COLOR);
            reactions_generator.generate(Vector(-0.6, 0.75, -0.8), Vector( 0.6, 1.75,  0.8), xyz_cells, this);

            // - Set impact -
            set_impact(Vector(0,1.5,0), 0.25, Vector(0, 0, 0), Vector(0,-110,0.0));
        }

    };
    const TCHAR * OvalDemo::cmdline_option = _T("/oval");
    const float4  OvalDemo::OVAL_COLOR (0.67f, 0.55f, 0.47f, 1);

    class DiffGeomDemo : public Demo
    {
    public:
        DiffGeomDemo(Application & app)
            : Demo(app, GLOBE_CURVE_VERTICES, GLOBE_CURVE_VERTICES, GLOBE_VERTICES, GLOBE_INDICES)
        {}

        static const TCHAR * cmdline_option;

    protected:
        // An extension to ICurve: a curve which can also be rendered after generating vertices for it
        class CurveWithVertices : public CrashAndSqueeze::Math::ICurve
        {
        protected:
            void check_t(Real t) const
            {
                if (t < T_START || t > T_END)
                {
                    throw OutOfRangeError(RT_ERR_ARGS("Curve parameter t out of range"));
                }
            }
            // When curve consists of multiple sub-curves, this function can be used
            // to split T_START..T_END interval into `parts_num` sub-intervals each having
            // its own parameter `s` in T_START..T_END.
            // Returns this `s` and sets `part_index` to the index of current sub-interval.
            Real partition(Real t, int parts_num, /*out*/ int & part_index) const
            {
                _ASSERT(parts_num >= 1);
                Real t_0_1 = (t - T_START)/(T_END-T_START); // normalize t to the interval 0..1
                part_index = std::min(static_cast<int>(t_0_1*parts_num), parts_num - 1); // = t*parts_num but no more than parts_num-1
                return T_START + (t_0_1*parts_num - part_index)*(T_END - T_START); // "inner" parameter `t` for i'th part
            }
        public:
            void make_vertices(Index vertices_num, const float4 &color, /*out*/ Vertex *res_vertices, /*out*/ Index *res_indices)
            {
                if (vertices_num < 2)
                    throw OutOfRangeError(RT_ERR_ARGS("Invalid number of vertices for CurveWithVertices"));

                Real dt = (T_END - T_START)/(vertices_num - 1);
                for (Index i = 0; i < vertices_num; ++i)
                {
                    Real t = T_START + i*dt;
                    res_vertices[i] = Vertex(math_vector_to_float3(point_at(t)), color, float3(0,0,0));
                    res_indices[i] = i;
                }
            }

            virtual string get_description() const = 0;
        };

        // The curves used to transport vector
        // They use spherical coordinates, so lines are arcs on a sphere!

        class LineSegment : public CurveWithVertices
        {
        private:
            Point start;
            Vector delta;
        public:
            LineSegment() : start(Vector::ZERO), delta(Vector::ZERO) {}
            LineSegment(Point start, Point end) : start(start), delta(end - start) {}

            virtual Point point_at(Real t) const override
            {
#ifndef NDEBUG
                check_t(t);
#endif //ifndef NDEBUG
                return start + (t - T_START)/(T_END-T_START)*delta;
            }

            virtual const Point & get_start() const { return start; }
            virtual Point get_end() const { return start + delta; }

            virtual string get_description() const override
            {
                return StringFormat() << "Segment from " << start << " to " << start+delta;
            }
        };

        // A "triangle" on the surface of sphere
        class TriangleCurve : public CurveWithVertices
        {
        private:
            LineSegment segments[3];
            string desc;
        public:
            TriangleCurve(Real r, Real theta_min, Real theta_max, Real phi_min, Real phi_max, const Vector &mini_offset = Vector(0,0,0.001))
            {
                Point left_point(r, theta_max, phi_min);
                Point top_point(r, theta_min, (phi_min+phi_max)/2);
                Point right_point(r, theta_max, phi_max);
                segments[0] = LineSegment(left_point-mini_offset, top_point);   // "/" side
                segments[1] = LineSegment(top_point, right_point);  // "\" side
                segments[2] = LineSegment(right_point, left_point+mini_offset); // "_" side
            }

            virtual Point point_at(Real t) const override
            {
#ifndef NDEBUG
                check_t(t);
#endif //ifndef NDEBUG
                int segment_id = 0;
                Real s = partition(t, 3, /*out*/ segment_id);
                return segments[segment_id].point_at(s);
            }

            virtual string get_description() const override
            {
                return StringFormat() << "Triangle " << segments[0].get_start() << "-" << segments[1].get_start() << "-" << segments[2].get_start();
            }

        };

        // A "parallelogram" which is both an ICurve and ISurface
        class Parallelogram : public CurveWithVertices, public CrashAndSqueeze::Math::ISurface
        {
        private:
            LineSegment segments[4];
            Point origin;
            Vector side1;
            Vector side2;
        public:
            // TODO: mini_offset is a kind of hack which depends on values of origin/side1/side2
            Parallelogram(const Point &origin, const Vector &side1, const Vector &side2, const Vector &mini_offset = Vector(0,0,-0.001))
                : origin(origin), side1(side1), side2(side2)
            {
                segments[0] = LineSegment(origin - mini_offset,   origin + side1);
                segments[1] = LineSegment(origin + side1,         origin + side1 + side2);
                segments[2] = LineSegment(origin + side1 + side2, origin + side2);
                segments[3] = LineSegment(origin + side2,         origin + mini_offset);
            }

            // Implement ICurve
            virtual Point point_at(Real t) const override
            {
#ifndef NDEBUG
                check_t(t);
#endif //ifndef NDEBUG
                int segment_id = 0;
                Real s = partition(t, 4, /*out*/ segment_id);
                return segments[segment_id].point_at(s);
            }

            // Implement ISurface
            virtual Point point_at(Real u, Real v) const override
            {
                return origin
                     + side1 * (u - U_START)/(U_END - U_START)
                     + side2 * (v - V_START)/(V_END - V_START);
            }

            virtual string get_description() const override
            {
                return StringFormat() << "Parallelogram " << segments[0].get_start() << "-" << segments[1].get_start() << "-" << segments[2].get_start() << "-" << segments[3].get_start();
            }

        };

        virtual void prepare() override
        {
            // A demo with vector transport in spherical coordinates
            const float globe_radius = 1;
            const float radius_coeff = 0.99f; // so that the line is visible well above sphere
            sphere(globe_radius*radius_coeff, float3(0,0,0), float4(0.1f,0.1f,0.1f,1), GLOBE_EDGES_PER_DIAMETER, high_model_vertices, high_model_indices);

            Model * globe = new Model(
                app.get_renderer(),
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
                simple_shader,
                high_model_vertices,
                GLOBE_VERTICES,
                high_model_indices,
                GLOBE_INDICES);
            globe->add_shader(lighting_shader);
            add_auxiliary_model(globe);

            // Define curve
            
            /*(1)
            LineSegment curve(Point(globe_radius, PI/2, 0), Point(globe_radius, PI/4, PI/2));
            */
            
            /*(2)
            Real triangle_size = PI/3; // height and half-width of triangle
            TriangleCurve curve(globe_radius, PI/2 - triangle_size/2, PI/2 + triangle_size/2, -triangle_size, triangle_size);
            */
            
            /*(3)*/
            Real square_size = PI/4;
            Parallelogram curve(Point(globe_radius, PI/2.5 + square_size/2, square_size/2 /*!!!*/ + PI),
                                Vector(0, -square_size, 0), Vector(0, 0, -square_size));
            

            /*(4)
            Real rhomb_size = PI/8;
            Real sqrt_2 = 1.41421356237309504880;
            Parallelogram curve(Point(globe_radius, PI/3 + rhomb_size/sqrt_2, rhomb_size/sqrt_2),
                                Vector(0, -rhomb_size/sqrt_2, -rhomb_size/sqrt_2), Vector(0, rhomb_size/sqrt_2, -rhomb_size/sqrt_2));
            */

            curve.make_vertices(GLOBE_CURVE_VERTICES, float4(0,0,0,1), low_model_vertices, low_model_indices);
            
            // Define initial vector
            Real vec_length = 0.8;//0.2;
            Vector vec = Vector(0, 1, 2).normalized()*vec_length;
            
            // Make parallel transport and update vertices
            
            ////!!! ISpace * space = new SphericalCoords;
            ISpace * space = new UnitSphere2D;
            DiscreteVectorField field(low_model_vertices, GLOBE_CURVE_VERTICES, VERTEX_INFO, space);
            Vector new_vec_along = field.transport_along(vec, &curve, 10000);
            field.update_vertices(low_model_vertices, VERTEX_INFO);

            // And show both curve and vectors
            Model * curve_model = new Model(
                app.get_renderer(),
                D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
                simple_shader,
                low_model_vertices,
                GLOBE_CURVE_VERTICES,
                low_model_indices,
                GLOBE_CURVE_VERTICES);
            curve_model->add_shader(simple_pixel_shader);
            add_auxiliary_model(curve_model);
            add_normals_model_for(curve_model, 1, false);
            set_camera_position(2.5f, DirectX::XM_PI/2, 0);

            // Compare with parallel transport using curvature tensor

            Vector new_vec_around = field.transport_around(vec, &curve, 100, 100);
            Vector pos = curve.point_at(CrashAndSqueeze::Math::ICurve::T_START);
            Vertex vec_vertices[2];
            vec_vertices[0].pos = math_vector_to_float3( space->point_to_cartesian(pos) + space->vector_to_cartesian(vec, pos) );
            vec_vertices[1].pos = math_vector_to_float3( space->point_to_cartesian(pos) + space->vector_to_cartesian(new_vec_around, pos) );
            vec_vertices[0].color = vec_vertices[1].color = float4(0,1,0, 1);
            Index vec_indices[2] = {0, 1};
            Model * vec_model = new Model(
                app.get_renderer(), D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, simple_shader,
                vec_vertices, 2, vec_indices, 2);
            vec_model->add_shader(simple_pixel_shader);
            add_auxiliary_model(vec_model);

            string msg = StringFormat() << "Result for transporting " << vec << (CAS_UPDATE_DURING_TRANSPORT?" WITH ":" WITHOUT ") << "v+=dv: " << curve.get_description() << ":";
            app.get_logger().log("        [DiffGeom]", msg.c_str());
            msg = StringFormat() << "Along (using Gijk):   " << new_vec_along << ", change = " << new_vec_along - vec << "dl = " << new_vec_along.norm() - vec.norm() << "(" << (new_vec_along.norm() - vec.norm())/vec.norm()*100 << "%)";
            app.get_logger().log("        [DiffGeom]", msg.c_str());
            msg = StringFormat() << "Around (using Rijkm): " << new_vec_around << ", change = " << new_vec_around - vec << "dl = "<< new_vec_around.norm() - vec.norm() << "(" << (new_vec_around.norm() - vec.norm())/vec.norm()*100 << "%)" << "; difference from above " << new_vec_around - new_vec_along;
            app.get_logger().log("        [DiffGeom]", msg.c_str());

/* the same, but find vector length using metric tensor of 2-sphere (which variant is right?)
            msg = StringFormat() << "Along (using Gijk):   " << new_vec_along << ", change = " << new_vec_along - vec << "dl(g) = " << g.norm(new_vec_along) - g.norm(vec) << "(" << (g.norm(new_vec_along) - g.norm(vec))/g.norm(vec)*100 << "%)";
            app.get_logger().log("        [DiffGeom]", msg.c_str());
            msg = StringFormat() << "Around (using Rijkm): " << new_vec_around << ", change = " << new_vec_around - vec << "dl(g) = "<< g.norm(new_vec_around) - g.norm(vec) << "(" << (g.norm(new_vec_around) - g.norm(vec))/g.norm(vec)*100 << "%)" << "; difference from above " << new_vec_around - new_vec_along;
*/
            /* == compare norm step-by-step
            const CrashAndSqueeze::Math::IMetric * metric = space->get_metric();
            for(int i = 0; i < field.get_nodes_count(); ++i)
            {
                const Vector & v = field.get_vector(i);
                CrashAndSqueeze::Math::MetricTensor g;
                metric->value_at(field.get_pos(i), g);
                msg = StringFormat() << v << " |v|=" << v.norm() << " / |v|g=" << g.norm(v);
                app.get_logger().log("        [DiffGeom]", msg.c_str());
            }*/

            // Coloring subshape demo
            CrashAndSqueeze::Core::VertexInfo color_vector_vinfo(
                sizeof(Vertex), 0, 24, false, 40, 48
                );
            // a piece of sphere surface
            const Index SUBSHAPE_VERTICES_COUNT = PLANE_VERTICES_COUNT;
            const Index SUBSHAPE_INDICES_COUNT  = PLANE_INDICES_COUNT;
            Vertex * subshape_vertices = new Vertex[SUBSHAPE_VERTICES_COUNT];
            Index * subshape_indices = new Index[SUBSHAPE_INDICES_COUNT];
            plane(float3(0, PI/4, 0), float3(0, 0, PI/4), float3(globe_radius, PI/2, 0), subshape_vertices, subshape_indices, float4(0.5f,0.5f,0.5f,1));

            DiscreteVectorField subshape_field(subshape_vertices, SUBSHAPE_VERTICES_COUNT, VERTEX_INFO, space);
            Parallelogram little_loop(Point(0,0,0), Vector(0, -PI/32, 0), Vector(0, 0, -PI/32));
            CrashAndSqueeze::Math::UniformVectorField same_vector(vec);
            subshape_field.transport_around_each(&same_vector, &little_loop, 10, 10);
            // store vector as color
            subshape_field.update_vertices(subshape_vertices, color_vector_vinfo, DiscreteVectorField::POS_TO_CARTESIAN);
            // store vector as normal
            subshape_field.update_vertices(subshape_vertices, VERTEX_INFO);
            Model * subshape_model = new Model(
                app.get_renderer(), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, simple_shader,
                subshape_vertices, SUBSHAPE_VERTICES_COUNT, subshape_indices, SUBSHAPE_INDICES_COUNT);
            subshape_model->add_shader(simple_pixel_shader);
            add_auxiliary_model(subshape_model);
            add_normals_model_for(subshape_model, 0.2, false);
            delete_array(subshape_vertices);
            delete_array(subshape_indices);
        }
    };
    const TCHAR * DiffGeomDemo::cmdline_option = _T("/diffgeom");
#pragma warning( pop )
}

// Renderer can be launched as `Renderer.exe file.obj` to load mesh from file.obj.
// Instead of file.obj one can pass /oval or /cylinder options to launch corresponding demo.
// If no argument is given, /oval is assumed by default.
INT WINAPI _tWinMain( HINSTANCE, HINSTANCE, LPTSTR, INT )
{
    Logger logger("renderer.log", true);
    logger.newline();
    logger.log("        [Renderer]", "application startup");

    PhysicsLogger & phys_logger = PhysicsLogger::get_instance();

    PhysLogAction phys_log_action(logger);
    PhysWarningAction phys_warn_action(logger);
    PhysErrorAction phys_err_action(logger);

    phys_logger.ignore(PhysicsLogger::LOG);
    // phys_logger.set_action(PhysicsLogger::LOG, &phys_log_action);
    phys_logger.set_action(PhysicsLogger::WARNING, &phys_warn_action);
    phys_logger.set_action(PhysicsLogger::ERROR, &phys_err_action);

    srand( static_cast<unsigned>( time(NULL) ) );

    try
    {
        Application app(logger);
        override_app_settings(app, PAINT_MODEL, SHOW_NORMALS, UPDATE_ON_GPU);

        // parse command line arguments
        tstring mesh_filename = DiffGeomDemo::cmdline_option; // by default use diffgeom demo
        if (__argc > 1)
            mesh_filename = __targv[1];

        if (mesh_filename == DiffGeomDemo::cmdline_option)
        {
            DiffGeomDemo demo(app);
            demo.run();
        }
        else if (mesh_filename == OvalDemo::cmdline_option)
        {
            OvalDemo demo(app);
            demo.run();
        }
        else if (mesh_filename == CylinderDemo::cmdline_option)
        {
            CylinderDemo demo(app, 0.5, 1);
            demo.run();
        }
        else
        {
            MeshDemo demo(app, mesh_filename);
            demo.run();
        }

        logger.log("        [Renderer]", "application shutdown");
    }
    catch(RuntimeError &e)
    {
        // NOTE: now resources are released automatically through Demo's virtual destructor!

        logger.log("ERROR!! [Renderer]", e.get_log_entry().c_str());
        logger.dump_messages();
        const TCHAR *MESSAGE_BOX_TITLE = _T("Renderer error!");
        my_message_box(e.get_message().c_str(), MESSAGE_BOX_TITLE, MB_OK | MB_ICONERROR, true);
        return -1;
    }
    return 0;
}
