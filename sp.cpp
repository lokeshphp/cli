// sp.cc  by Andrew Goldberg.

#define SINGLE_PAIR

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "sp.h"
#include "pch.h"

void ArcLen(long cNodes, Node *nodes,
	    long long *pMin /* = NULL */, long long *pMax /* = NULL */, SP *sp)
// finds the max arc length and min arc length of a graph.
// Used to init for buckets.  pMax or pMin can be NULL.  For fun,
// and utility, returns MaxArcLen.
{
   Arc2 *lastArc, *arc;
   long long maxLen = 0, minLen = VERY_FAR;
//	 int NodNr;

            // arcs are stored sequentially.  The last arc overall
            // is one before the first arc of the sentinel node.
   lastArc = (nodes+cNodes)->first - 1;
   for ( arc = nodes->first; arc <= lastArc; arc++ )
   {
//		 if(sp != NULL){
//			 NodNr = sp->nodeId(arc->head);
//			 fprintf(stdout, "Nod %d ", NodNr);
//		 }
//		 fprintf(stdout, "Bagkostn %lld\n", arc->len);
	   if(arc->len < 0){
		   errlog(0,"ERROR: arc cost %d<0, ends at node %d\n", arc->len,
			   sp->nodeId(arc->head));
	   }
      if ( arc->len > maxLen )
	 maxLen = arc->len;
      if ( arc->len < minLen )
         minLen = arc->len;
//	  if (arc == nodes->first)
//		  fprintf(stdout, "Forsta bage, cost %I64d, maxLen %I64d, minLen %I64d\n", arc->len, maxLen, minLen);
   }
//   fprintf(stdout, "ArcLen sista bage cost %I64d, maxLen %I64d, minLen %I64d\n", arc->len, maxLen, minLen);
   if (*pMin)
	   *pMin = minLen;
   else
	   fprintf(stdout, "ERROR! pMin ej def\n");
   if(*pMax)
	   *pMax  = maxLen;
   else
	   fprintf(stdout, "ERROR! pMax ej def\n");

}


//-------------------------------------------------------------
// SP::SP()
// SP::~SP()
//     Each algorithm needs a different data structure.  We add
//     them all here and then send them in the sp() call.  This
//     way we need to allocate memory only once even though there
//     may be many sp calls.  Note that we expect the sp calls
//     all to be made on the same graph, though info about it
//     may change.
//-------------------------------------------------------------

SP::SP(long cNodesGiven, Node *nodesGiven, ulong levels, ulong logDelta,
       bool doBFS)
{
  long long minArcLen = -1, maxArcLen = -1;

  cNodes = cNodesGiven;
  nodes = nodesGiven;
	tries = 1;
  cCalls = cScans = cUpdates = 0;     // no stats yet
  tries = 0;
  smartq = NULL;                      // for DIK_SMARTQ
  BFSqueue = NULL;
#ifdef SINGLE_PAIR
  curTime = 0;
#endif
  //** initialize data type for new sp algorithm here **//

  ArcLen(cNodes, nodes, &minArcLen, &maxArcLen, this);
  spType = SP_DIK_SMARTQ;               // no need to store bucket #
  if (!doBFS)
    smartq = new SmartQ(&minArcLen,
			&maxArcLen,
			levels, logDelta,
			cNodes, nodes);
  else {
    BFSqueue = new Bucket;
    BFSqueue->pNode = NULL;
  }
}

SP::~SP()
{
//   if ( smartq )      delete smartq;
   if ( smartq )      smartq->~SmartQ();
   if (BFSqueue) delete BFSqueue;
//** delete data structure for new sp algorithm here **//
}

//-------------------------------------------------------------
// SP::initNode()
//     Initially, all distances are set to VERY_FAR, presumably
//     longer than any real distance
//-------------------------------------------------------------
#ifdef SINGLE_PAIR
void SP::initNode(Node *currentNode, unsigned long ts)
#else
void SP::initNode(Node *currentNode)
#endif
{
   currentNode->where = IN_NONE;   // nodes not in any data structure yet
   currentNode->dist = VERY_FAR;   // not yet a shortest path
   currentNode->sBckInfo.bucket = NULL;
#ifdef SINGLE_PAIR
   currentNode->tStamp = ts;
#endif
}

//-------------------------------------------------------------
// SP::init()
//     Initially, all lengths are set to VERY_FAR, presumably
//     longer than any real distance, and the parent field is
//     set to NULL to indicate the sp tree does not yet exist.
//-------------------------------------------------------------
void SP::init(Node *source)
{
   Node *currentNode;
   long iNode;             // to make our loop faster

   for ( currentNode=nodes, iNode=0; iNode <= cNodes; iNode++, currentNode++ )
   {
#ifdef SINGLE_PAIR
     initNode(currentNode, 0);
#else
     initNode(currentNode);
#endif
   }

   source->parent = source;
//	 source->NodNr = NodNr;
   source->dist = 0;               // all distances are to the source
}

// less work; for base line timing
void SP::BFSInit(Node *source)
{
   Node *currentNode;
   long iNode;             // to make our loop faster

   for ( currentNode=nodes, iNode=0; iNode <= cNodes; iNode++, currentNode++ )
   {
      currentNode->dist = VERY_FAR;   // not yet a shortest path
      currentNode->parent = NULL;
   }

   source->dist = 0;               // all distances are to the source
//	 source->NodNr = NodNr;
}

//-------------------------------------------------------------
// SP::sp()
//     Just calls the SP algorithm specified in spType, and returns
//     the overall stat that the SP algorithm returns.  Actual
//     code for sp algorithms can be found in their cc files:
//     dijkstra.cc, etc.
//        If OPEN is not NULL, we test all arcs with OPEN and
//     ignore those that aren\'t OPEN.  Otherwise all ARCS are open
//        If StopAt is not NULL, we do shortest paths only until
//     we reach a node for which StopAt returns 1 (it may check
//     for nodes with excess capacity, for instance).  Thus,
//     only part of the distance tree will be correct, but that
//     part will include a path from StopAt to a source.
//        RETURNS the node that StopAt stopped at, or NULL if
//     StopAt was NULL or no node passed StopAt\'s test.
//-------------------------------------------------------------
#ifdef SINGLE_PAIR
bool SP::spTmp1()
{
    cCalls++;

    tries++;
    printf("ok\n");
    return FALSE;
    //return (model, smartq->dijkstra(model, source, sink, this, OptCost));
}

bool SP::spTmp2(strModel model)
{
    cCalls++;

    tries++;
    printf("ok\n");
    return FALSE;
    //return (model, smartq->dijkstra(model, source, sink, this, OptCost));
}

bool SP::spTmp3(Node* source, Node* sink)
{
    cCalls++;

    tries++;
    printf("ok\n");
    return FALSE;
    //return (model, smartq->dijkstra(model, source, sink, this, OptCost));
}

bool SP::spTmp(strModel model, Node* source, Node* sink, long long* OptCost)
{
    cCalls++;

    tries++;
    printf("ok\n");
    return FALSE;
    //return (model, smartq->dijkstra(model, source, sink, this, OptCost));
}

bool SP::sp(strModel model, Node *source, Node *sink, long long *OptCost)
{
  cCalls++;

	tries++;
  return (model, smartq->dijkstra(model, source, sink, this, OptCost));
}
#else
//bool SP::sp(Node *source, Node *sink, long long *OptCost)
//{
//  cCalls++;

//	tries++;
//  return (smartq->dijkstra(source, sink, this, OptCost));
//}
void SP::sp(Node *source)
{
  cCalls++;

   smartq->dijkstra(source, this);
}
#endif
//-------------------------------------------------------------
// SP::PrintStats()
//     Prints stats appropriate for an sp run.  First it prints
//     stats that hold for all sp algorithms: number of times
//     it's called, number of scans (nodes looked at), number
//     of updates (times a node's distance changes).  Then it
//     calls the appropriate data structure to print out
//     algorithm-specific stats.
//-------------------------------------------------------------
#ifdef SINGLE_PAIR
void SP::PrintStats(long tries2)
{
	tries++;
   printf("c Scans avg.: %21ld       Improvements avg.: %12ld\n", 
	  cScans / tries, 
	  cUpdates / tries);
   smartq->PrintStats();
}
#else
void SP::PrintStats()
{
   printf("c Scans: %26ld       Improvements: %17ld\n", cScans, cUpdates);
   smartq->PrintStats();
}
#endif

void SP::initStats()
{
  cScans = cUpdates = tries = 0;
}

int SP::nodeId(Node *i)

{
  return (int)(i - nodes + 1);
}

// for baseline timing
// do BFS

long SP::BFS(Node *source)

{
  Node *v, *w;
  Arc2 *a, *stopA;
  long nFound = 0;

  BFSInit(source);
  BFSqueue->pNode = source;
  source->sBckInfo.next = source->sBckInfo.prev = source;
  do {
    nFound++;
    v = BFSqueue->pNode;

    // scan v
    stopA = (v+1)->first - 1;
    for (a = v->first; a <= stopA; a++) {
      w = a->head;
      if (w->dist == VERY_FAR) {
	w->dist = v->dist + 1;
	w->parent = v;

	// insert w
	v->sBckInfo.prev->sBckInfo.next = w;
	w->sBckInfo.prev = v->sBckInfo.prev;
	v->sBckInfo.prev = w;
	w->sBckInfo.next = v;
      }
    }

    // delete v
    if (v->sBckInfo.next != v) {
      BFSqueue->pNode = v->sBckInfo.next;
      v->sBckInfo.next->sBckInfo.prev = v->sBckInfo.prev;
      v->sBckInfo.prev->sBckInfo.next = v->sBckInfo.next;
    }
    else {
      BFSqueue->pNode = NULL;
    }
  } while (BFSqueue->pNode != NULL);
  return (nFound);
}
