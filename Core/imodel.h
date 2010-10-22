#pragma once
#include "Core/physical_vertex.h"
#include "Core/cluster.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An interface of model, having vertices and clusters
        class IModel
        {
        public:
            virtual int get_vertices_num() const = 0;
            virtual const PhysicalVertex & get_vertex(int index) const = 0;
            
            virtual int get_clusters_num() const = 0;
            virtual const Cluster & get_cluster(int index) const = 0;
        };
    }
}
