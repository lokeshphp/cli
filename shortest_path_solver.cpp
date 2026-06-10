

#include "pch.h"

#include <iostream>
#include <vector>
#include <queue>
#include <limits>
#include <algorithm>
#include <unordered_set>
#include <fstream>
#include <chrono>

using namespace std;

const double INF = numeric_limits<double>::infinity();

struct Edge {
    int to;
    double weight;
    Edge(int _to, double _weight) { to = _to; weight = _weight; };
};

struct State {
    double dist;
    int node;

    bool operator>(const State& other) const {
        return dist > other.dist;
    }
};

class Graph {
private:
    int V;
    vector<vector<Edge>> adj;
    vector<vector<Edge>> radj;

public:
    Graph(int vertices) : V(vertices), adj(vertices), radj(vertices) {}

    // OPTIMIZATION 1: Reserve capacity upfront if you know average degree
    void reserveCapacity(int avgDegree) {
        for (int i = 0; i < V; i++) {
            adj[i].reserve(avgDegree);
            radj[i].reserve(avgDegree);
        }
        cout << "Reserved capacity for ~" << avgDegree << " edges per node\n";
    }

    // Standard single edge insertion
    void addEdge(int from, int to, double weight) {
        adj[from].push_back({ to, weight });
        radj[to].push_back({ from, weight });
    }

    // OPTIMIZATION 2: Bulk edge insertion - much faster!
    // Edges format: vector of tuples (from, to, weight)
    void addEdgesBulk(const vector<tuple<int, int, double>>& edges) {
        // Count edges per node first
        vector<int> forwardCount(V, 0);
        vector<int> backwardCount(V, 0);

        for (const auto& [from, to, weight] : edges) {
            forwardCount[from]++;
            backwardCount[to]++;
        }

        // Reserve exact capacity
        for (int i = 0; i < V; i++) {
            adj[i].reserve(adj[i].size() + forwardCount[i]);
            radj[i].reserve(radj[i].size() + backwardCount[i]);
        }

        // Now add all edges
        for (const auto& [from, to, weight] : edges) {
            adj[from].push_back({ to, weight });
            radj[to].push_back({ from, weight });
        }
    }

    // OPTIMIZATION 3: Fast file reading with optimized I/O
    void loadEdgesFromFile(const string& filename) {
        // Disable sync with C stdio for faster I/O
        ios_base::sync_with_stdio(false);
        cin.tie(nullptr);

        ifstream file(filename);
        if (!file) {
            cerr << "Error opening file: " << filename << "\n";
            return;
        }

        vector<tuple<int, int, double>> edges;
        edges.reserve(10000000); // Reserve for ~10M edges, adjust as needed

        int from, to;
        double weight;
        while (file >> from >> to >> weight) {
            edges.emplace_back(from, to, weight);
        }

        cout << "Read " << edges.size() << " edges from file\n";
        addEdgesBulk(edges);
    }

    // OPTIMIZATION 4: Direct construction from edge list
    static Graph fromEdgeList(int vertices, const vector<tuple<int, int, double>>& edges) {
        Graph g(vertices);
        g.addEdgesBulk(edges);
        return g;
    }

    // Standard Dijkstra with early termination - stops when target is reached
    pair<double, vector<int>> dijkstraWithTarget(int source, int target) {
        vector<double> dist(V, INF);
        vector<int> parent(V, -1);
        priority_queue<State, vector<State>, greater<State>> pq;

        dist[source] = 0;
        pq.push({ 0, source });

        while (!pq.empty()) {
            State current = pq.top();
            pq.pop();

            int u = current.node;
            int d = current.dist;

            // Early termination - found target!
            if (u == target) {
                vector<int> path;
                for (int v = target; v != -1; v = parent[v]) {
                    path.push_back(v);
                }
                reverse(path.begin(), path.end());
                return { dist[target], path };
            }

            if (d > dist[u]) continue;

            for (const Edge& edge : adj[u]) {
                int v = edge.to;
                double weight = edge.weight;

                if (dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                    parent[v] = u;
                    pq.push({ dist[v], v });
                }
            }
        }

        return { INF, vector<int>() };  // unreachable
    }

    // Bidirectional Dijkstra (same as before)
    pair<double, vector<int>> bidirectionalDijkstra(int source, int target) {
        if (source == target) {
            return { 0, vector<int>{source} };
        }

        vector<double> distF(V, INF);
        vector<int> parentF(V, -1);
        priority_queue<State, vector<State>, greater<State>> pqF;
        unordered_set<int> processedF;

        vector<double> distB(V, INF);
        vector<int> parentB(V, -1);
        priority_queue<State, vector<State>, greater<State>> pqB;
        unordered_set<int> processedB;

        distF[source] = 0;
        pqF.push({ 0, source });

        distB[target] = 0;
        pqB.push({ 0, target });

        double bestDist = INF;
        int meetNode = -1;

        while (!pqF.empty() || !pqB.empty()) {
            if (!pqF.empty()) {
                State current = pqF.top();
                pqF.pop();

                int u = current.node;
                int d = current.dist;

                if (d > bestDist) {
                    if (pqB.empty() || pqB.top().dist > bestDist) break;
                }

                if (processedF.count(u)) continue;
                processedF.insert(u);

                if (distB[u] != INF) {
                    int pathDist = distF[u] + distB[u];
                    if (pathDist < bestDist) {
                        bestDist = pathDist;
                        meetNode = u;
                    }
                }

                for (const Edge& edge : adj[u]) {
                    int v = edge.to;
                    int weight = edge.weight;

                    if (distF[u] + weight < distF[v]) {
                        distF[v] = distF[u] + weight;
                        parentF[v] = u;
                        pqF.push({ distF[v], v });
                    }
                }
            }

            if (!pqB.empty()) {
                State current = pqB.top();
                pqB.pop();

                int u = current.node;
                int d = current.dist;

                if (d > bestDist) {
                    if (pqF.empty() || pqF.top().dist > bestDist) break;
                }

                if (processedB.count(u)) continue;
                processedB.insert(u);

                if (distF[u] != INF) {
                    int pathDist = distF[u] + distB[u];
                    if (pathDist < bestDist) {
                        bestDist = pathDist;
                        meetNode = u;
                    }
                }

                for (const Edge& edge : radj[u]) {
                    int v = edge.to;
                    int weight = edge.weight;

                    if (distB[u] + weight < distB[v]) {
                        distB[v] = distB[u] + weight;
                        parentB[v] = u;
                        pqB.push({ distB[v], v });
                    }
                }
            }
        }

        if (meetNode == -1) {
            return { INF, vector<int>() };
        }

        vector<int> pathForward;
        for (int v = meetNode; v != -1; v = parentF[v]) {
            pathForward.push_back(v);
        }
        reverse(pathForward.begin(), pathForward.end());

        vector<int> pathBackward;
        for (int v = parentB[meetNode]; v != -1; v = parentB[v]) {
            pathBackward.push_back(v);
        }

        vector<int> fullPath = pathForward;
        fullPath.insert(fullPath.end(), pathBackward.begin(), pathBackward.end());

        return { bestDist, fullPath };
    }

    void createGraph_slow(strModel* model) {
        int i, i1, tail, head;
        double weight;

        auto tid0 = std::chrono::high_resolution_clock::now();
        vector<int> backwardCount(V, 0);

        auto tid1 = std::chrono::high_resolution_clock::now();
        for (i = 0; i < model->nNoder; i++) {
            for (i1 = 0; i1 < model->Noder[i].nUtNoder; i1++) {
                backwardCount[model->Noder[i].UtNod[i1]]++;
            }
        }
        auto tid2 = std::chrono::high_resolution_clock::now();
        
        // Reserve exact capacity
        for (int i = 0; i < V; i++) {
            adj[i].reserve(adj[i].size() + model->Noder[i].nUtNoder);
            radj[i].reserve(radj[i].size() + backwardCount[i]);
        }
        auto tid3 = std::chrono::high_resolution_clock::now();

        for (i = 0; i < model->nNoder; i++) {
            for (i1 = 0; i1 < model->Noder[i].nUtNoder; i1++) {
                head = model->Noder[i].UtNod[i1];
                weight = model->Noder[i].UtNodCost[i1];
                //adj[i].push_back({ head, weight });
                //radj[head].push_back({ i, weight });
                adj[i].emplace_back(head, weight);
                radj[head].emplace_back(i, weight);
            }
        }
        auto tid4 = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> fp_ms1 = tid1 - tid0;
        std::chrono::duration<double, std::milli> fp_ms2 = tid2 - tid1;
        std::chrono::duration<double, std::milli> fp_ms3 = tid3 - tid2;
        std::chrono::duration<double, std::milli> fp_ms4 = tid4 - tid3;
        errlog("loop fast ny alloc %lf count %lf reserve %lf addValues %lf\n", fp_ms1, fp_ms2, fp_ms3, fp_ms4);

        // return g;
    }

    void createGraph_medium(strModel* model, int nExtra) {
        int i, i1, tail, head;

        auto tid0 = std::chrono::high_resolution_clock::now();
        reserveCapacity(model->nArcs / model->nNoder + nExtra);
        auto tid1 = std::chrono::high_resolution_clock::now();
        for (i = 0; i < model->nNoder; i++) {
            for (i1 = 0; i1 < model->Noder[i].nUtNoder; i1++) {
                tail = i;
                head = model->Noder[i].UtNod[i1];
                addEdge(tail, head, model->Noder[i].UtNodCost[i1]);
                //edges.emplace_back(tail, head, model->Noder[i].UtNodCost[i1]);
            }
        }
        auto tid2 = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> fp_ms2 = tid1 - tid0;
        std::chrono::duration<double, std::milli> fp_ms3 = tid2 - tid1;
        errlog("loop medium nExtra %d reserve %lf loop %lf claude\n", nExtra, fp_ms2, fp_ms3);

        // return g;
    }

    void createGraph_claude(strModel* model) {
        int i, i1, tail, head;

        auto tid0 = std::chrono::high_resolution_clock::now();
        vector<tuple<int, int, double>> edges;
        edges.reserve(model->nArcs); // Reserve for ~10M edges, adjust as needed

        // Graph g(model->nArcs);
        auto tid1 = std::chrono::high_resolution_clock::now();

        for (i = 0; i < model->nNoder; i++) {
            for (i1 = 0; i1 < model->Noder[i].nUtNoder; i1++) {
                tail = i;
                head = model->Noder[i].UtNod[i1];
                // g.addEdge(tail, head, model->Noder[i].UtNodCost[i1]);
                edges.emplace_back(tail, head, model->Noder[i].UtNodCost[i1]);
            }
        }
        auto tid2 = std::chrono::high_resolution_clock::now();

        addEdgesBulk(edges);
        auto tid3 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> fp_ms1 = tid1 - tid0;
        std::chrono::duration<double, std::milli> fp_ms2 = tid2 - tid1;
        std::chrono::duration<double, std::milli> fp_ms3 = tid3 - tid2;
        errlog("reserve %lf loop %lf bulk %lf claude\n", fp_ms1, fp_ms2, fp_ms3);

        // return g;
    }
};

int main2() {
    cout << "=== GRAPH CONSTRUCTION PERFORMANCE COMPARISON ===\n\n";

    const int NODES = 100000;
    const int EDGES = 500000;

    // Generate sample edges
    vector<tuple<int, int, double>> edgeList;
    edgeList.reserve(EDGES);
    for (int i = 0; i < EDGES; i++) {
        int from = rand() % NODES;
        int to = rand() % NODES;
        double weight = 1.0 + (rand() % 10000) / 100.0;  // Random weight between 1.0 and 101.0
        edgeList.emplace_back(from, to, weight);
    }

    // Method 1: Individual insertion (SLOW)
    cout << "Method 1: Adding edges one by one...\n";
    auto start = chrono::high_resolution_clock::now();
    Graph g1(NODES);
    for (const auto& [from, to, weight] : edgeList) {
        g1.addEdge(from, to, weight);
    }
    auto end = chrono::high_resolution_clock::now();
    auto duration1 = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Time: " << duration1.count() << " ms\n\n";

    // Method 2: With capacity reservation (FASTER)
    cout << "Method 2: With capacity reservation...\n";
    start = chrono::high_resolution_clock::now();
    Graph g2(NODES);
    g2.reserveCapacity(10); // Assume ~10 edges per node
    for (const auto& [from, to, weight] : edgeList) {
        g2.addEdge(from, to, weight);
    }
    end = chrono::high_resolution_clock::now();
    auto duration2 = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Time: " << duration2.count() << " ms\n";
    cout << "Speedup: " << (float)duration1.count() / duration2.count() << "x\n\n";

    // Method 3: Bulk insertion (FASTEST)
    cout << "Method 3: Bulk edge insertion...\n";
    start = chrono::high_resolution_clock::now();
    Graph g3(NODES);
    g3.addEdgesBulk(edgeList);
    end = chrono::high_resolution_clock::now();
    auto duration3 = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Time: " << duration3.count() << " ms\n";
    cout << "Speedup: " << (float)duration1.count() / duration3.count() << "x\n\n";

    // Test shortest path still works
    cout << "=== TESTING SHORTEST PATH ===\n";
    auto [dist, path] = g3.bidirectionalDijkstra(0, 1000);
    if (dist != INF) {
        cout << "Found path of length " << dist << " with " << path.size() << " nodes\n";
    }

    cout << "\n=== RECOMMENDATIONS FOR YOUR OCEAN NETWORK ===\n";
    cout << "1. Use addEdgesBulk() - 5-10x faster than individual addEdge()\n";
    cout << "2. If reading from file, use loadEdgesFromFile() for optimized I/O\n";
    cout << "3. Pre-allocate edge vector with .reserve() before loading\n";
    cout << "4. For 1M nodes + few million edges, expect construction in < 1 second\n";

    return 0;
}



int solve_dijkstras_claude(strModel* model) {

    auto tid0 = std::chrono::high_resolution_clock::now();
    Graph g(model->nNoder);
    auto tid1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms1 = tid1 - tid0;
    errlog("init graph claude %lf\n", fp_ms1);
    g.createGraph_claude(model);
    auto tid2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms2 = tid2 - tid0;
    errlog("set up network claude tot %lf\n", fp_ms2);

    tid0 = std::chrono::high_resolution_clock::now();
    Graph g1(model->nNoder);
    tid1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms1b = tid1 - tid0;
    errlog("init slow graph claude %lf\n", fp_ms1b);
    g1.createGraph_slow(model);
    tid2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms2b = tid2 - tid0;
    errlog("set up network slow claude tot %lf\n", fp_ms2b);

    tid0 = std::chrono::high_resolution_clock::now();
    Graph g2(model->nNoder);
    tid1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms1c = tid1 - tid0;
    errlog("init medium graph claude %lf\n", fp_ms1c);
    g2.createGraph_medium(model, 0);
    tid2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms2c = tid2 - tid0;
    errlog("set up network medium 0 claude tot %lf\n", fp_ms2c);

    int source = 0;
    int target = model->nNoder - 1;

    std::cout << "=== STANDARD DIJKSTRA WITH EARLY TERMINATION ===\n";
    tid1 = std::chrono::high_resolution_clock::now();
    auto [dist1, path1] = g.dijkstraWithTarget(source, target);

    tid2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms3 = tid2 - tid1;
    errlog("solve standard dijkstra with claude %lf\n", fp_ms3);

    std::cout << "Distance: " << dist1 << "\n";
    std::cout << "Path: ";
    for (int i = 0; i < path1.size(); i++) {
        std::cout << path1[i];
        if (i < path1.size() - 1) std::cout << " -> ";
    }
    std::cout << "\n\n";

    std::cout << "=== BIDIRECTIONAL DIJKSTRA WITH EARLY TERMINATION ===\n";
    tid1 = std::chrono::high_resolution_clock::now();
    auto [dist2, path2] = g.bidirectionalDijkstra(source, target);
    // auto [dist1, path1] = g.dijkstraWithTarget(source, target);

    tid2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms4 = tid2 - tid1;
    errlog("solve dijkstra bidirectional with claude %lf\n", fp_ms4);

    std::cout << "Distance: " << dist2 << "\n";
    std::cout << "Path: ";
    for (int i = 0; i < path2.size(); i++) {
        std::cout << path2[i];
        if (i < path2.size() - 1) std::cout << " -> ";
    }
    std::cout << "\n\n";




    exit(0);

    return 0;
}

