//#include <bits/stdc++.h>
#include<random>
#include "pch.h"
#include <queue>
using namespace std;

using ll = long long;
static const ll INF = (1LL << 60);

struct Edge {
    int to;
    int w; // edge cost (must be >=0 for both methods; must be integer for Dial)
};

struct Graph {
    int n = 0;
    vector<vector<Edge>> adj;
    bool directed = true;
    int Wmax = 0; // maximum edge weight seen (useful for Dial)

    Graph() = default;
    Graph(int n_, bool directed_) : n(n_), adj(n_), directed(directed_) {}

    void add_edge(int u, int v, int w) {
        // nonnegative requirement for Dijkstra
        if (w < 0) 
            errlog("Negative edge weight: from %d to %d cost %d Dijkstra is not valid.\n", u, v, w);
        adj[u].push_back({v, w});
        if (!directed) adj[v].push_back({u, w});
        if (w > Wmax) Wmax = w;
    }
};

// ---------- Path reconstruction ----------
vector<int> reconstruct_path(int s, int t, const vector<int>& parent) {
    // parent[v] = predecessor of v on chosen shortest path tree, or -1
    if (s == t) return {s};
    if (t < 0 || t >= (int)parent.size()) return {};

    vector<int> path;
    for (int v = t; v != -1; v = parent[v]) {
        path.push_back(v);
        if (v == s) break;
    }
    if (path.back() != s) return {}; // unreachable
    reverse(path.begin(), path.end());
    return path;
}

// ---------- 1) Heap-based Dijkstra (general nonnegative weights) ----------
struct DijkstraResult {
    vector<ll> dist;
    vector<int> parent;
};

DijkstraResult dijkstra_heap(const Graph& g, int src) {
    int n = g.n;
    DijkstraResult res;
    res.dist.assign(n, INF);
    res.parent.assign(n, -1);

    using P = pair<ll,int>; // (dist, node)
    priority_queue<P, vector<P>, greater<P>> pq;

    res.dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d != res.dist[u]) continue; // stale entry

        for (const auto& e : g.adj[u]) {
            int v = e.to;
            ll nd = d + (ll)e.w;
            if (nd < res.dist[v]) {
                res.dist[v] = nd;
                res.parent[v] = u;
                pq.push({nd, v});
            }
        }
    }
    return res;
}

// ---------- 2) Dial¡¯s algorithm (bucket Dijkstra) ----------
/*
Requirements:
- All edge weights are integers in [0..Wmax].
- Wmax should be reasonably small for performance.

We implement a fast circular-bucket version:
- buckets size = Wmax+1
- we allow duplicates and use the "dist == currentDist" stale check
- we also use a settled[] array to finalize nodes once (like Dijkstra)
*/
DijkstraResult dijkstra_dial(const Graph& g, int src, int Wmax) {
    if (Wmax < 0) throw runtime_error("Wmax must be nonnegative.");
    int n = g.n;

    DijkstraResult res;
    res.dist.assign(n, INF);
    res.parent.assign(n, -1);

    if (Wmax == 0) {
        // all edges weight 0 -> BFS-like with queue
        deque<int> dq;
        res.dist[src] = 0;
        dq.push_back(src);
        vector<char> inq(n, 0);
        inq[src] = 1;
        while (!dq.empty()) {
            int u = dq.front(); dq.pop_front();
            inq[u] = 0;
            for (auto &e : g.adj[u]) {
                int v = e.to;
                ll nd = res.dist[u] + e.w; // e.w must be 0
                if (nd < res.dist[v]) {
                    res.dist[v] = nd;
                    res.parent[v] = u;
                    if (!inq[v]) { dq.push_back(v); inq[v] = 1; }
                }
            }
        }
        return res;
    }

    const int B = Wmax + 1;
    vector<vector<int>> buckets(B);
    vector<char> settled(n, 0);

    res.dist[src] = 0;
    buckets[0].push_back(src);

    ll currentDist = 0;
    int settledCount = 0;

    while (true) {
        // Find next non-empty bucket by advancing currentDist.
        // In Dial, you¡¯ll never need to advance more than Wmax between settled nodes,
        // but duplicates/stale entries can cause extra pops (still usually fast).
        int spins = 0;
        while (buckets[(int)(currentDist % B)].empty()) {
            currentDist++;
            if (++spins > (ll)Wmax + 5) {
                // If we scanned more than ~Wmax without finding anything,
                // it often means remaining nodes are unreachable.
                // But duplicates can complicate this, so we do a robust check:
                bool any = false;
                for (int i = 0; i < B; ++i) if (!buckets[i].empty()) { any = true; break; }
                if (!any) return res;
                // Otherwise keep scanning
                spins = 0;
            }
        }

        int idx = (int)(currentDist % B);
        int u = buckets[idx].back();
        buckets[idx].pop_back();

        if (res.dist[u] != currentDist) continue; // stale
        if (settled[u]) continue;

        settled[u] = 1;
        settledCount++;
        if (settledCount == n) return res;

        for (const auto& e : g.adj[u]) {
            // Safety: Dial requires e.w <= Wmax
            if (e.w < 0 || e.w > Wmax) {

                errlog("Dial's algorithm: edge weight outside [0..Wmax], is %d\n", e.w);
            }
            int v = e.to;
            ll nd = currentDist + (ll)e.w;
            if (nd < res.dist[v]) {
                res.dist[v] = nd;
                res.parent[v] = u;
                buckets[(int)(nd % B)].push_back(v);
            }
        }
    }
}

// ---------- Input / Output helpers ----------
Graph read_graph_from_stdin() {
    /*
      Input format:
        n m directed(0/1)
        m lines: u v w
      Nodes assumed 0-based (0..n-1).
    */
    int n, m, dirFlag;
    if (!(cin >> n >> m >> dirFlag)) throw runtime_error("Failed to read n m directed.");
    Graph g(n, dirFlag == 1);

    g.adj.reserve(n);
    for (int i = 0; i < m; ++i) {
        int u, v, w;
        cin >> u >> v >> w;
        if (u < 0 || u >= n || v < 0 || v >= n) throw runtime_error("Edge endpoint out of range.");
        g.add_edge(u, v, w);
    }
    return g;
}

void print_path(const vector<int>& path) {
    if (path.empty()) {
        cout << "(no path)\n";
        return;
    }
    for (int i = 0; i < (int)path.size(); ++i) {
        if (i) cout << " ";
        cout << path[i];
    }
    cout << "\n";
}

Graph load_graph_from_model(strModel* model) {
    int i, i1, tail, head;
    int n = model->nNoder, dirFlag = 1;
    int m = model->nArcs, costInt;
    Graph g(n, dirFlag == 1);

    g.adj.reserve(n);
    for (i = 0; i < model->nNoder; i++) {
        for (i1 = 0; i1 < model->Noder[i].nUtNoder; i1++) {
            tail = i;
            head = model->Noder[i].UtNod[i1];
            if (model->Noder[i].UtNodCost[i1] > INT_MAX)
                costInt = INT_MAX;
            else
                costInt = model->Noder[i].UtNodCost[i1];
            g.add_edge(tail, head, costInt);
        }
    }

    return g;
}


int solve_dijkstras_chatGPT(strModel* model){
    //ios::sync_with_stdio(false);
    //cin.tie(nullptr);

    //try {
        //Graph g = read_graph_from_stdin();
        auto tid0 = std::chrono::high_resolution_clock::now();
        Graph g = load_graph_from_model(model);
        auto tid1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> fp_ms1 = tid1 - tid0;
        errlog("set up network chatGPT %lf\n", fp_ms1);

        int s = 0, t = model->nNoder - 1;
        if (s < 0 || s >= g.n || t < 0 || t >= g.n) errlog("Source/target out of range.");

        // --- Heap Dijkstra ---
        auto heapRes = dijkstra_heap(g, s);
        auto tid2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> fp_ms2 = tid2 - tid1;
        errlog("solve dijkstra heap with chatGPT %lf\n", fp_ms2);

        cout << "Heap Dijkstra:\n";
        if (heapRes.dist[t] >= INF/2) {
            cout << "  dist = INF (unreachable)\n";
            cout << "  path = ";
            print_path({});
        } else {
            cout << "  dist = " << heapRes.dist[t] << "\n";
            cout << "  path = ";
            print_path(reconstruct_path(s, t, heapRes.parent));
        }

        // --- Dial Dijkstra (only if suitable) ---
        // Condition: integer weights already satisfied by type; need Wmax not too large for performance.
        // For correctness, it works for any Wmax, but can be slow if huge.
        cout << "\nDial (bucket) Dijkstra:\n";
        int Wmax = g.Wmax;
        cout << "  Using Wmax = " << Wmax << "\n";

        auto tid2b = std::chrono::high_resolution_clock::now();
        auto dialRes = dijkstra_dial(g, s, Wmax);
        auto tid3 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> fp_ms3 = tid3 - tid2b;
        errlog("solve dijkstra bucket with chatGPT %lf\n", fp_ms3);


        if (dialRes.dist[t] >= INF/2) {
            cout << "  dist = INF (unreachable)\n";
            cout << "  path = ";
            print_path({});
        } else {
            cout << "  dist = " << dialRes.dist[t] << "\n";
            cout << "  path = ";
            print_path(reconstruct_path(s, t, dialRes.parent));
        }

        // Optional sanity check: distances should match (when Dial is applicable)
        if (heapRes.dist[t] != dialRes.dist[t]) {
            cout << "\nWARNING: Heap and Dial distances differ for target.\n"
                 << "This can happen if Dial's preconditions were violated (e.g., weights > Wmax or overflow),\n"
                 << "or if you have extremely large weights and hit implementation limits.\n";
        }

    //} catch (const exception& ex) {
    //    cerr << "Error: " << ex.what() << "\n";
    //    return 1;
    //}

    return 0;
}
