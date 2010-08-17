#pragma once
#include "physical_vertex.h"
#include "cluster.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class Model
        {
        private:
            PhysicalVertex vertices;
            int vertices_num;

            Cluster *clusters;
            int clusters_num;
        };
    }
}