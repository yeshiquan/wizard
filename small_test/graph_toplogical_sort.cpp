#include <iostream>
#include <vector>

struct Node {
    Node(int v) : value(v) {}
    int value = 0;
    int deps = 0;
    std::vector<Node*> inputs;
    std::vector<Node*> outputs;

    void add_input(Node* node) {
        inputs.emplace_back(node);
    }
    void add_output(Node* node) {
        outputs.emplace_back(node);
        node->add_input(this);
    }
};

class Graph {
public:
    void add_vertex(Node* node) {
        vertexs.emplace_back(node);
    }

    void print() {
        std::cout << "Graph Vertex: ";
        for (auto node : vertexs) {
            std::cout << node->value << ",";
        }
        std::cout << std::endl;
    }

    void toplogical_sort_forward() {
        std::vector<Node*> sorted;
        for (auto node : vertexs) {
            node->deps = node->inputs.size();
            if (node->deps == 0) {
                sorted.emplace_back(node);
            }
        }
        int i = 0;
        while (i < sorted.size()) {
           Node* node = sorted[i++];
           for (auto o : node->outputs) {
               o->deps--;
               if (o->deps == 0) {
                   sorted.push_back(o);
               }
           }
        }

        std::cout << "Graph Toplogical Forward: ";
        for (auto node : sorted) {
            std::cout << node->value << ",";
        }
        std::cout << std::endl;
    }

    void toplogical_sort_backward() {
        std::vector<Node*> sorted;
        for (auto node : vertexs) {
            node->deps = node->outputs.size();
            if (node->deps == 0) {
                sorted.emplace_back(node);
            }
        }
        int i = 0;
        while (i < sorted.size()) {
           Node* node = sorted[i++];
           for (auto o : node->inputs) {
               o->deps--;
               if (o->deps == 0) {
                   sorted.push_back(o);
               }
           }
        }

        std::cout << "Graph Toplogical Backward: ";
        for (auto node : sorted) {
            std::cout << node->value << ",";
        }
        std::cout << std::endl;
    }

private:
    std::vector<Node*> vertexs;
};

int main() {
    Graph graph;

    Node* node1 = new Node(1);
    Node* node2 = new Node(2);
    Node* node3 = new Node(3);
    Node* node4 = new Node(4);
    Node* node5 = new Node(5);
    Node* node6 = new Node(6);

    node1->add_output(node3);
    node1->add_output(node4);

    node2->add_output(node4);
    node2->add_output(node5);

    node3->add_output(node6);
    node4->add_output(node6);
    node5->add_output(node6);

    graph.add_vertex(node1);
    graph.add_vertex(node2);
    graph.add_vertex(node3);
    graph.add_vertex(node4);
    graph.add_vertex(node5);
    graph.add_vertex(node6);

    graph.print();
    graph.toplogical_sort_forward();
    graph.toplogical_sort_backward();

	return 0;
}
