#include "pch.h"

long long MAXVARDE_NATVERK = 10000000000000000; // 100000000000;
int skrivUtWarning = 0;

int SattUppDijkstraNatverk3(strModel* model) {
	//	int NodNr, antal;
	//	Arc2 *arc;
	int n, m, i1, NodNr = 0, Forsta = 0, taMedBage;
	long long minArcLen = -1, maxArcLen = -1, length;
	double dist;
	Node* nodes = NULL, * nod = NULL;            /* pointer to the node structure */
	Arc2* arcs = NULL, * lastArc = NULL, * arc = NULL;             /* pointer to the arc structure */
	Arc2* arc_current = NULL;
	Arc2* arc_new;
	SP* sp = NULL;
	model->filpek = NULL;

	long    node_min = 0,               /* minimal no of node  */
		node_max = 0,               /* maximal no of nodes */
		* arc_first = NULL,              /* internal array for holding
										- node degree
										- position of the first outgoing arc */
		* arc_tail = NULL;               /* internal array: tails of the arcs */

	long head, tail, i;
	long last, arc_num, arc_new_num;
	int nBagarNatv = 0;


	//FILE* pek;
	//pek = fopen("checkDijkst3.txt", "w");
	//for (int i = 0; i < model->nNoder; i++) {
	//	for (int i1 = 0; i1 < model->Noder[i].nUtNoder; i1++) {
	//		fprintf(pek, "i %d i1 %d head %d\n", i, i1, model->Noder[i].UtNod[i1]);
	//	}
	//}
	//fclose(pek);

	n = model->nNoder; 
	m = model->nArcs; //  model->nArcsOK;
	errlog("Dijkstra network, nNodes %d, nArcs %d\n", n, m);
	double maxCost = 0, minCost = 1e10;
	for (i = 0; i < m; i++) {
		if (model->arc[i].totCost > maxCost)
			maxCost = model->arc[i].totCost;
		if (model->arc[i].totCost < minCost)
			minCost = model->arc[i].totCost;
	}
	if (maxCost > 0) {
		model->Dijkstra.FAKTOR_NATVERK = (long long)(MAXVARDE_NATVERK / maxCost);
		//if (model->Dijkstra.FAKTOR_NATVERK > 1e10)
			model->Dijkstra.FAKTOR_NATVERK = 1000;
	}

	//errlog("MaxCost in network is %lf which gives FAKTOR_NATVERK %lf.\n MinCost is %.2lf\n",
	//	maxCost, model->Dijkstra.FAKTOR_NATVERK, minCost);
	if(skrivUtWarning == 1)
		errlog("nNodes %d, nArcs %d\n", n, m);

	if (model->Dijkstra.nodes != NULL) {
		delete model->Dijkstra.sp;
		free(model->Dijkstra.nodes);
		free(model->Dijkstra.arcs);
	}

	/* allocating memory for  'nodes', 'arcs'  and internal arrays */
	nodes = (Node*)calloc(n + 2, sizeof(Node));
	if (nodes == NULL) printf("Ups1\n");
	arcs = (Arc2*)calloc(m + 1, sizeof(Arc2));
	if (arcs == NULL) printf("Ups12\n");
	arc_tail = (long*)calloc(m, sizeof(long));
	if (arc_tail == NULL) printf("Ups13\n");
	arc_first = (long*)calloc(n + 2, sizeof(long));
	if (arc_first == NULL) printf("Ups14\n");
	/* arc_first [ 0 .. n+1 ] = 0 - initialized by calloc */

	if (nodes == NULL || arcs == NULL ||
		arc_first == NULL || arc_tail == NULL)
		/* memory is not allocated */
	{
		printf("Need %lld bytes for data and %lld bytes temp. data\n",
			((long long)(n + 2)) * ((long long)sizeof(Node)) +
			((long long)(m + 1)) * ((long long)sizeof(Arc2)),
			((long long)(n + m + 2)) * ((long long)sizeof(long)));
		printf("n %d, m %d storl Node %d, storl Arc2 %d, storl long %d\n",
			n, m, sizeof(Node), sizeof(Arc2), sizeof(long));
	}

	/* setting pointer to the current arc */
	arc_current = arcs;

	node_max = 0;
	node_min = n;

	int saveDijkstraNetwork = 0;
	if (saveDijkstraNetwork == 1) {
		FILE* pek = fopen("checkDijkst4.txt", "w");
		for (int i = 0; i < model->nNoder; i++) {
			for (int i1 = 0; i1 < model->Noder[i].nUtNoder; i1++) {
				fprintf(pek, "i %d i1 %d head %d arcNr %d from %d %d %d to %d %d %d cost %.3lf\n", i, i1, model->Noder[i].UtNod[i1],
					model->Noder[i].outArcNr[i1], model->arc[model->Noder[i].outArcNr[i1]].fromLevel,
					model->arc[model->Noder[i].outArcNr[i1]].fromTime, model->arc[model->Noder[i].outArcNr[i1]].fromPointNr,
					model->arc[model->Noder[i].outArcNr[i1]].toLevel, model->arc[model->Noder[i].outArcNr[i1]].toTime,
					model->arc[model->Noder[i].outArcNr[i1]].toPointNr, model->Noder[i].UtNodCost[i1]);
			}
		}
		fclose(pek);
	}

	//pek = fopen("checkDijkst.txt", "w");
	checkMinnesAnvandning(__LINE__);

	//	model->OmvandlDijkstraToNodeNr = (int*)calloc(model->nNoder, sizeof(int));
	maxCost = 0;
	minCost = 1e30;
	for (i = 0; i < model->nNoder; i++) {
		if (i == 21)
			i = i;
		//		model->OmvandlDijkstraToNodeNr[i] = 1;
		//fprintf(pek, "i %d nUtNoder %d nBagarNatv %d node_min %d node_max %d\n", i, model->Noder[i].nUtNoder, nBagarNatv, node_min, node_max);
		for (i1 = 0; i1 < model->Noder[i].nUtNoder; i1++) {
			//			model->OmvandlDijkstraToNodeNr[model->Noder[i]->UtNod[i1]] = 1;
			//			fprintf(stdout, "FranNod %d, tillnod %d, cost %.3lf\n",i, model->Noder[i]->UtNod[i1], 
			//				model->Noder[i]->UtNodCost[i1]);

			taMedBage = 1;
			if (taMedBage == 1) {
				tail = i;
				head = model->Noder[i].UtNod[i1];
				//fprintf(pek, "i1 %d head %d\n", i1, head);
				if (tail >= n || head >= n)
					i = i;
				length = (long long)(model->Noder[i].UtNodCost[i1] * model->Dijkstra.FAKTOR_NATVERK);

				if (length < 0) {
					printf("ERROR: arc fran nod %d till nodpos %d har neg kostn %I64d utNodCost %.2lf, andrar den till 1e16, ",
						i, i1, length, model->Noder[i].UtNodCost[i1]);
					errlog(0, "ERROR: arc fran nod %d till nodpos %d har neg kostn %I64d, andrar den till 1e16, ",
						i, i1, length);
					fprintf(stdout, "utnodcost %.3lf faktor %.3lf\n",
						model->Noder[i].UtNodCost[i1], model->Dijkstra.FAKTOR_NATVERK);

					length = 10000000000000000;
				}
				//printf("arcNr %d franTillNoder %d %d cost %I64d\n",
				//	nBagarNatv, tail, head, length);

				//if (i== 10145 &&head == 11863)
				//	errlog("error: arc from nod %d to %d cost %I64d\n", i, head, length);

				arc_first[tail + 1] ++; /* no of arcs outgoing from tail
										is stored in arc_first[tail+1] */

										/* storing information about the arc */
				arc_tail[nBagarNatv] = tail;
				arc_current->head = nodes + head;
				arc_current->len = length;

				if (length > maxCost)
					maxCost = length;
				if (length < minCost)
					minCost = length;


				/* searching minimumu and maximum node */
				if (head < node_min) node_min = head;
				if (tail < node_min) node_min = tail;
				if (head > node_max) node_max = head;
				if (tail > node_max) node_max = tail;
				if (node_max > 4000)
					node_max = node_max;
				nBagarNatv++;
				arc_current++;
			}
		}
	}

	(nodes + node_min)->first = arcs;

	// printf("antal bagar in natverk till Dijkstras %d\n", nBagarNatv);

	/* before below loop arc_first[i+1] is the number of arcs outgoing from i;
	after this loop arc_first[i] is the position of the first
	outgoing from node i arcs after they would be ordered;
	this value is transformed to pointer and written to node.first[i]
	*/


	//errlog("node_min %d node_max %d\n", node_min, node_max);
	for (i = node_min + 1; i <= node_max + 1; i++)
	{
		//			 fprintf(stdout, "ArcFirst %d: %d\n", i, arc_first[i]);
		//			 NodNr = sp->nodeId(nodes + i);
		//			 fprintf(stdout, "Nodb %d\n", NodNr);

		//fprintf(pek, "i %d first %d first-1 %d\n", i, arc_first[i], arc_first[i - 1]);
		arc_first[i] += arc_first[i - 1];
		(nodes + i)->first = arcs + arc_first[i];
	}
	//fclose(pek);

	for (i = node_min; i < node_max; i++) /* scanning all the nodes
										  exept the last*/
	{

		last = (long)(((nodes + i + 1)->first) - arcs);
		/* arcs outgoing from i must be cited
		from position arc_first[i] to the position
		equal to initial value of arc_first[i+1]-1  */

		for (arc_num = arc_first[i]; arc_num < last; arc_num++)
		{
			tail = arc_tail[arc_num];

			while (tail != i)
				/* the arc no  arc_num  is not in place because arc cited here
				must go out from i;
				we'll put it to its place and continue this process
				until an arc in this position would go out from i */

			{
				Arc2 arc_tmp;        /* used in swapping below */
				arc_new_num = arc_first[tail];
				arc_current = arcs + arc_num;
				arc_new = arcs + arc_new_num;

				//			 NodNr = sp->nodeId(arc_current->head);
				//			 fprintf(stdout, "NodD %d\n", NodNr);
				//			 NodNr = sp->nodeId(arc_new->head);
				//			 fprintf(stdout, "NodE %d\n", NodNr);

				/* arc_current must be cited in the position arc_new
				swapping these arcs:                                 */

				arc_tmp.head = arc_new->head;
				arc_new->head = arc_current->head;
				arc_current->head = arc_tmp.head;

				arc_tmp.len = arc_new->len;
				arc_new->len = arc_current->len;
				arc_current->len = arc_tmp.len;

				arc_tail[arc_num] = arc_tail[arc_new_num];

				/* we increase arc_first[tail] but label previous position */

				arc_tail[arc_new_num] = tail;
				arc_first[tail] ++;

				tail = arc_tail[arc_num];
			}
		}
		/* all arcs outgoing from  i  are in place */
	}

	/* -----------------------  arcs are ordered  ------------------------- */



	//	sp = new SP(n, nodes + node_min, 
	//		0, 0, true);
	//	antal = 0;
	//  for ( arc= nodes->first; antal<= 10; arc++ )
	//   {
	//		 NodNr = sp->nodeId(arc->head);
	//		 fprintf(stdout, "Nod %d arc adr %d\n", NodNr, (long long)arc);
	//		 antal++;
	//	 }

	/* assigning output values */
	model->Dijkstra.nArcs = m;
	model->Dijkstra.nNoder = node_max - node_min + 1;
	model->Dijkstra.node_min = node_min;
	model->Dijkstra.nodes = nodes + node_min;
	//errlog("I SattUppDijkstraNatverk3 %I64d\n", (long long)model->Dijkstra.nodes);

	model->Dijkstra.arcs = arcs;

	model->Dijkstra.cLevels = 0;
	model->Dijkstra.logDelta = 0;
	model->Dijkstra.doBFS = false;

	model->Dijkstra.sp = new SP(model->Dijkstra.nNoder, model->Dijkstra.nodes,
		model->Dijkstra.cLevels, model->Dijkstra.logDelta,
		model->Dijkstra.doBFS);

	ArcLen(model->Dijkstra.nNoder, model->Dijkstra.nodes,
		&minArcLen, &maxArcLen, model->Dijkstra.sp);      // other useful stats
	model->Dijkstra.minArcLen = minArcLen;
	model->Dijkstra.maxArcLen = maxArcLen;

	// sanity check
	dist = (double)model->Dijkstra.maxArcLen * (double)(model->Dijkstra.nNoder - 1);
	if (skrivUtWarning == 1) {
		if (dist > VERY_FAR) {
			fprintf(stderr, "Warning: distances may overflow\n");
			fprintf(stderr, "         proceed at your own risk!\n");
			fprintf(stderr, "         maxArcLen %I64d nNoder %d ger %.10e och veryFar ar %I64d\n",
				model->Dijkstra.maxArcLen, model->Dijkstra.nNoder, dist,
				VERY_FAR);
		}
	}

	/* free internal memory */
	free(arc_first); 
	free(arc_tail);


	/* Uff! all is done */
	return (0);
}


int ChangeArcCosts3(strModel* model) {
	double dist;
	long long minArcLen = -1, maxArcLen = -1;
	Arc2* arc;
	long long length;
	int i, i1, taMedBage;//, NodNr, Forsta = 0;
	//	 Node *nod;
	//	 Arc2 *lastArc;


	arc = model->Dijkstra.nodes->first;
	for (i = 0; i < model->nNoder; i++) {
		for (i1 = 0; i1 < model->Noder[i].nUtNoder; i1++) {
			taMedBage = 1;
			if (taMedBage == 1) {
				length = (long long)(model->Noder[i].UtNodCost[i1] * model->Dijkstra.FAKTOR_NATVERK);
				if (length < 0)
					errlog("ERROR: negativ kostnad for nod %d pos %d, flyttal %.4lf, heltal %I64d\n",
						i, i1, model->Noder[i].UtNodCost[i1] * model->Dijkstra.FAKTOR_NATVERK, length);
				if (length == 0)
					length = 1;
				arc->len = length;
				arc++;
			}
		}
	}
	if (arc > (model->Dijkstra.nodes + model->Dijkstra.nNoder)->first)
		errlog("ERROR: fler bagkostnader andrade an vad det finns bagar... rad %d\n",
			__LINE__);

	model->Dijkstra.sp = new SP(model->Dijkstra.nNoder, model->Dijkstra.nodes,
		model->Dijkstra.cLevels, model->Dijkstra.logDelta,
		model->Dijkstra.doBFS);

	ArcLen(model->Dijkstra.nNoder, model->Dijkstra.nodes,
		&minArcLen, &maxArcLen, model->Dijkstra.sp);      // other useful stats

	model->Dijkstra.minArcLen = minArcLen;
	model->Dijkstra.maxArcLen = maxArcLen;

	// sanity check
	dist = (double)model->Dijkstra.maxArcLen * (double)(model->Dijkstra.nNoder - 1);
	if (skrivUtWarning == 1){
		if (dist > VERY_FAR) {
			fprintf(stderr, "Warning: distances may overflow\n");
			fprintf(stderr, "         proceed at your own risk!\n");
			fprintf(stderr, "         maxArcLen %I64d nNoder %d ger %.10e och veryFar ar %I64d\n",
				model->Dijkstra.maxArcLen, model->Dijkstra.nNoder, dist,
				VERY_FAR);
		}
	}

	/*
	FILE *FilPek;
	FilPek = fopen("NatverkCheck.txt", "w");
	for(nod = model->Dijkstra->nodes; nod < model->Dijkstra->nodes+model->Dijkstra->nNoder; nod++){
	lastArc = (nod+1)->first - 1;
	Forsta = 0;
	NodNr = model->Dijkstra->sp->nodeId(nod)+model->Dijkstra->node_min-1;
	for(arc = nod->first; arc <= lastArc; arc++){
	if(Forsta == 0){
	fprintf(FilPek, "nod %d (verkl %d, adr %ld), bagar:\n",
	NodNr, model->Noder[NodNr]->NodID, (long)nod);
	Forsta = 1;
	}
	NodNr = model->Dijkstra->sp->nodeId(arc->head)+model->Dijkstra->node_min-1;
	fprintf(FilPek, "\tbagadr %ld baglangd %d till nod %d (verkl %d, adr %ld)\n",
	(long) arc, (int)arc->len, NodNr, model->Noder[NodNr]->NodID, (long)(arc->head));
	}
	}
	fclose(FilPek);
	*/
	return 0;
}



int AnropDijkstra2(int NodA, int NodB, strModel *model, bool *Reached) {
	Node *source;
	long long OptCost = 0;

	source = model->Dijkstra.nodes -
		model->Dijkstra.node_min + NodA;

	Node *sink;
	Node *currentNode;

	sink = model->Dijkstra.nodes -
		model->Dijkstra.node_min + NodB;

	if (NodA != NodB) {
		for (currentNode = model->Dijkstra.nodes;
			currentNode < model->Dijkstra.nodes + model->Dijkstra.nNoder;
			currentNode++)
			currentNode->tStamp = 0;

		*Reached = model->Dijkstra.sp->sp(*model, source, sink, &OptCost); //, maxCost);
		model->Dijkstra.OptCost = (long long)(OptCost / model->Dijkstra.FAKTOR_NATVERK);
	}
	else
		*Reached = model->Dijkstra.sp->sp(*model, source, source, &OptCost); //, maxCost);

	return 0;
}


double NystaUppBV_MassTest(strModel* model, int Reached, int NodA0, int NodB0, long long* Cost) {
	int i, i1, nNoder = 0, Nod1, Nod2, ArcPos, ArcNr, NodNu;
	int i11, VerklBage, ArcNr2;
	Node* source, * sink, * newNode;
	double TotCost = 0;
	double dist = 0;
	FILE* FilPek = NULL;
	
	source = model->Dijkstra.nodes -
		model->Dijkstra.node_min + NodA0;
	sink = model->Dijkstra.nodes -
		model->Dijkstra.node_min + NodB0;

	if (sink->dist > VERY_FAR / 1.1) {
		model->nBVArcs = 0;
		return (double)(source->dist);
	}
	
	for (newNode = sink; newNode != source; newNode = newNode->parent) {
		// NodNu = model->Dijkstra.sp->nodeId(newNode) + model->Dijkstra.node_min;
		NodNu = model->Dijkstra.sp->nodeId(newNode) + model->Dijkstra.node_min;
		//printf("uppnystning baklanges nNoder %d nodNu %d objCost %I64d\n", nNoder, NodNu - 1,
		//	newNode->dist);
		if (NodNu - 1 >= model->nNoder)
			printf("ERROR! NodNr %d to high, max %d\n", NodNu, model->nNoder);
		if (nNoder >= model->nNoder)
			errlog("ERROR! Rundgang i uppnystningen kodrad %d\n", __LINE__);
		model->BVtempNodOrder[nNoder] = NodNu - 1;
		if(model->filpek != NULL)
			fprintf(model->filpek, "pos %d nodNr %d costDijkstra %I64d\n", nNoder, model->BVtempNodOrder[nNoder],
				newNode->dist);
		//errlog("(pos %d )n%d(p%d): c %.2lf\n", nNoder, NodNu-1, nNoder, newNode->dist / model->Dijkstra.FAKTOR_NATVERK);
		nNoder++;

	}
	//checkMinnesAnvandning(__LINE__);
	// nNoder--;

	NodNu = model->Dijkstra.sp->nodeId(newNode) + model->Dijkstra.node_min;
	model->BVtempNodOrder[nNoder] = NodNu - 1;
	if (nNoder >= 0) {
		int nod1, nod2;
		for (i = 0; i < nNoder; i++) {
			nod1 = model->BVtempNodOrder[nNoder - i];
			nod2 = model->BVtempNodOrder[nNoder - i - 1];
			//checkMinnesAnvandning(__LINE__);
			for (i1 = 0; i1 < model->Noder[nod1].nUtNoder; i1++) {
				if (model->Noder[nod1].UtNod[i1] == nod2)
					break;
			}
			if (i1 < model->Noder[nod1].nUtNoder) {
				model->BVArc[i] = model->Noder[nod1].outArcNr[i1];
				//printf("arcPos %d arcNr %d distArc %.2lf cost %I64d noder %d %d\n", i, model->BVArc[i],
				//	model->arc[model->BVArc[i]].distance,
				//	(long long)(model->Noder[nod1].UtNodCost[i1] * model->Dijkstra.FAKTOR_NATVERK),
				//	nod1, nod2);
				dist += model->arc[model->BVArc[i]].distance;
				TotCost += model->arc[model->BVArc[i]].totCost;
				if (model->filpek != NULL)
					fprintf(model->filpek, "pos %d arcNr %d costDijkstra %.3lf totCost %.3lf\n", i, model->BVArc[i],
						TotCost);
				//errlog("pos %d arcNr %d dist %.2lf totDist %.2lf cost %.2lf totCost %I64d\n", i, model->BVArc[i], 
				//	model->arc[model->BVArc[i]].distance, dist,
				//	model->arc[model->BVArc[i]].totCost, TotCost);
			}
			else {
				errlog("ERROR! Could not find the arc that connects nodes %d and %d\n", nod1, nod2);
				model->BVArc[i] = 0;
			}
		}
		model->nBVArcs = i;
		*Cost = TotCost;
	}
	else {
		*Cost = 9999999;
	}
	//checkMinnesAnvandning(__LINE__);

	//int pos = 0;
	//for (newNode = source; ; newNode++) {
	//	NodNu = model->Dijkstra.sp->nodeId(newNode) + model->Dijkstra.node_min - 1;
	//	printf("pos %d NodNu %d objCost %I64d\n", pos, NodNu,
	//		newNode->dist);
	//	if (nNoder >= model->nNoder)
	//		errlog("ERROR! Rundgang i uppnystningen kodrad %d\n", __LINE__);
	//	if (newNode == sink)
	//		break;
	//	pos++;
	//}
	return dist;
}


int testAnrop(strModel* model, int nod1) {
	int nod2, nReached = 0;
	Node* source, * sink;
	bool Reached;

	//nod1 = model->nNoder - 1;
	//nod2 = model->nNoder - 1;
	nod1 = 0;
	nod2 = 0;
	AnropDijkstra2(nod1, nod2, model, &Reached);
	source = model->Dijkstra.nodes -
		model->Dijkstra.node_min + nod1;

	for (int i = 0; i < model->nNoder; i++) {
		sink = model->Dijkstra.nodes -
			model->Dijkstra.node_min + i;
		if (sink->dist < VERY_FAR)
			nReached++;
	}

	nod1 = model->nNoder - 1;
	nod2 = model->nNoder - 1;
	AnropDijkstra2(nod1, nod2, model, &Reached);
	source = model->Dijkstra.nodes -
		model->Dijkstra.node_min + nod1;

	for (int i = 0; i < model->nNoder; i++) {
		sink = model->Dijkstra.nodes -
			model->Dijkstra.node_min + i;
		if (sink->dist < VERY_FAR)
			nReached++;
	}



	nod1 = (int)(model->nNoder / 2);
	AnropDijkstra2(nod1, nod2, model, &Reached);
	source = model->Dijkstra.nodes -
		model->Dijkstra.node_min + nod1;

	nod1 = model->nNoder - 2;
	AnropDijkstra2(nod1, nod2, model, &Reached);
	source = model->Dijkstra.nodes -
		model->Dijkstra.node_min + nod1;

	return 0;
}

