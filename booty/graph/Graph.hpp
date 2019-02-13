#pragma once

/* Namespace graph implements generic graph data structures and algorithms.
 * @Simoncqk - 2019.02
 */

#include<string>
#include<unordered_map>

namespace booty {

	namespace graph {

		// Id of each graph node, id should be unique.
		template<typename T>
		struct ID {
			using type = T;
			T id;
		};

		// Node interface defination.
		template<typename IdType = int>
		class NodeInterface {
		public:
			// return id of current node(vertex).
			IdType ID() = 0;
			// return a string-typed description of current node(vertex).
			std::string String() = 0;
		};

		// Edge interface defination.
		template<typename IdType = int>
		class EdgeInterface {

			using WeightType = double;

		public:
			NodeInterface<IdType> Source() = 0;
			NodeInterface<IdType> Target() = 0;
			double Weight() = 0;
			std::string String() = 0;
		};

		// Graph interface defination, including all basic but significant operations
		// for a graph structure, it is the main entry for graph lib.
		template<typename IdType = int>
		class GraphInterface {
		public:
			using NodeMap = std::unordered_map<IdType, NodeInterface<IdType>>;

			// total number of nodes.
			int NodeCounts() = 0;
			// get node by the given id.
			NodeInterface<IdType> GetNode(IdType id) = 0;
			// return nodes with its ids.
			NodeMap GetNodes() = 0;
			// add new node, return true if add succeed, else false if add fails
			// or node has existed.
			bool AddNode(const NodeInterface<IdType>& node) = 0;
			// delete node, return true if deletion succeed, else false if deletion fails
			// or node has been deleted.
			bool DeleteNode(IdType id) = 0;
			// add new edge, return true if add succeed, else false if add fails
			// or edge has existed.
			bool AddEdge(IdType srcId, IdType tgtId, double weight) = 0;
			// update edge, return true if update succeed, else false if update fails
			// or edge does not exist.
			bool UpdateEdge(IdType srcId, IdType tgtId, double weight) = 0;
			// delete edge, return true if deletion succeed, else false if deletion fails
			// or edge has been deleted.
			bool DeleteEdge(IdType srcId, IdType tgtId) = 0;
			// get weight of edge from node(srcId) to node(tgtId).
			double GetWeight(IdType srcId, IdType tgtId) = 0;
			// get all previous nodes of the given id node.
			NodeMap GetSources(IdType id) = 0;
			// get all target nodes of the given id node.
			NodeMap GetTargets(IdType id) = 0;
		};

		// Default graph edge implementation.
		template<typename IdType = int>
		class Edge {
		public:
			explicit Edge(const NodeInterface<IdType>& src, const NodeInterface<IdType>& tgt, double weight)
				:src(src), tgt(tgt), weight(weight) {}
			NodeInterface<IdType> Source() {
				return src;
			}
			NodeInterface<IdType> Target() {
				return tgt;
			}
			double Weight() {
				return weight;
			}
			std::string String() {
				char buf[256];
				// TODO: add weight info
				std::fprintf(buf, "%s -> %s, weight: %lf\n", src.String(), tgt.String(), weight);
				return buf;
			}
			// override < operator for sorting.
			bool operator < (const Edge& e) {
				return this->weight < e.weight;
			}
		private:
			NodeInterface<IdType> src;
			NodeInterface<IdType> tgt;
			double weight;
		};

		// Default graph edge implementation.
		template<typename IdType=int>
		class Graph {

		};

	} // namespace graph

} // namespace booty
