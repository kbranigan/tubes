
#define DELETED -2

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION convert_to_voronoi
#include "scheme.h"

struct  Freenode {
  struct  Freenode  *nextfree;
};
struct  Freelist {
  struct  Freenode  *head;
  int      nodesize;
};
char *getfree();
void *malloc(size_t size);
char *myalloc();

float xmin, xmax, ymin, ymax, deltax, deltay;
float pxmin, pxmax, pymin, pymax, cradius;
int triangulate, sorted, plot, debug;

struct Point {
  float x,y;
};

struct Site {
  struct  Point  coord;
  int    sitenbr;
  int    refcnt;
};

struct  Site *sites;
int    nsites;
int    siteidx;
int    sqrt_nsites;
//int    nvertices;
struct   Freelist sfl;
struct  Site  *bottomsite;

struct Edge {
  double    a,b,c;
  struct  Site   *ep[2];
  struct  Site  *reg[2];
  int    edgenbr;
};
#define le 0
#define re 1

int nedges;
struct  Freelist efl;

int has_endpoint(),right_of();
struct Site *intersect();
float dist();
struct Point PQ_min();
struct Halfedge *PQextractmin();
struct Edge *bisect();

struct Halfedge {
  struct Halfedge  *ELleft, *ELright;
  struct Edge  *ELedge;
  int    ELrefcnt;
  char    ELpm;
  struct  Site  *vertex;
  double    ystar;
  struct  Halfedge *PQnext;
};

struct   Freelist  hfl;
struct  Halfedge *ELleftend, *ELrightend;
int   ELhashsize;
struct  Halfedge **ELhash;
struct  Halfedge *HEcreate(), *ELleft(), *ELright(), *ELleftbnd();
struct  Site *leftreg(), *rightreg();

int PQhashsize;
struct  Halfedge *PQhash;
struct  Halfedge *PQfind();
int PQcount;
int PQmin;
int PQempty();

int ntry, totalsearch;

ELinitialize()
{
  int i;
  freeinit(&hfl, sizeof **ELhash);
  ELhashsize = 2 * sqrt_nsites;
  ELhash = (struct Halfedge **) myalloc ( sizeof *ELhash * ELhashsize);
  for(i=0; i<ELhashsize; i +=1) ELhash[i] = (struct Halfedge *)NULL;
  ELleftend = HEcreate( (struct Edge *)NULL, 0);
  ELrightend = HEcreate( (struct Edge *)NULL, 0);
  ELleftend->ELleft = (struct Halfedge *)NULL;
  ELleftend->ELright = ELrightend;
  ELrightend->ELleft = ELleftend;
  ELrightend->ELright = (struct Halfedge *)NULL;
  ELhash[0] = ELleftend;
  ELhash[ELhashsize-1] = ELrightend;
}

struct Halfedge *HEcreate(struct Edge *e, int pm)
{
  struct Halfedge *answer;
  answer = (struct Halfedge *) getfree(&hfl);
  answer->ELedge = e;
  answer->ELpm = pm;
  answer->PQnext = (struct Halfedge *) NULL;
  answer->vertex = (struct Site *) NULL;
  answer->ELrefcnt = 0;
  return(answer);
}

ELinsert(struct  Halfedge *lb, struct  Halfedge *new)
{
  new->ELleft = lb;
  new->ELright = lb->ELright;
  (lb->ELright)->ELleft = new;
  lb->ELright = new;
}

/* Get entry from hash table, pruning any deleted nodes */
struct Halfedge *ELgethash(int b)
{
  struct Halfedge *he;

  if(b<0 || b>=ELhashsize) return((struct Halfedge *) NULL);
  he = ELhash[b]; 
  if (he == (struct Halfedge *) NULL || 
      he->ELedge != (struct Edge *) DELETED ) return (he);

  /* Hash table points to deleted half edge.  Patch as necessary. */
  ELhash[b] = (struct Halfedge *) NULL;
  if ((he->ELrefcnt -= 1) == 0) makefree(he, &hfl);
  return ((struct Halfedge *) NULL);
}  

struct Halfedge *ELleftbnd(struct Point *p)
{
  int i, bucket;
  struct Halfedge *he;

  /* Use hash table to get close to desired halfedge */
  bucket = (p->x - xmin)/deltax * ELhashsize;
  if(bucket<0) bucket =0;
  if(bucket>=ELhashsize) bucket = ELhashsize - 1;
  he = ELgethash(bucket);
  if(he == (struct Halfedge *) NULL)
  {
    for(i=1; 1 ; i += 1)
    {
      if ((he=ELgethash(bucket-i)) != (struct Halfedge *) NULL) break;
      if ((he=ELgethash(bucket+i)) != (struct Halfedge *) NULL) break;
    }
    totalsearch += i;
  }
  ntry += 1;
/* Now search linear list of halfedges for the corect one */
  if (he==ELleftend  || (he != ELrightend && right_of(he,p)))
  {
    do {
      he = he->ELright;
    } while (he!=ELrightend && right_of(he,p));
    he = he->ELleft;
  }
  else
    do {
      he = he->ELleft;
    } while (he!=ELleftend && !right_of(he,p));

/* Update hash table and reference counts */
  if(bucket > 0 && bucket <ELhashsize-1)
  {
    if(ELhash[bucket] != (struct Halfedge *) NULL) ELhash[bucket]->ELrefcnt -= 1;
    ELhash[bucket] = he;
    ELhash[bucket]->ELrefcnt += 1;
  }
  return (he);
}

/* This delete routine can't reclaim node, since pointers from hash
   table may be present.   */
ELdelete(struct Halfedge *he)
{
  (he->ELleft)->ELright = he->ELright;
  (he->ELright)->ELleft = he->ELleft;
  he->ELedge = (struct Edge *)DELETED;
}

struct Halfedge *ELright(struct Halfedge *he)
{
  return (he->ELright);
}

struct Halfedge *ELleft(struct Halfedge *he)
{
  return (he->ELleft);
}

struct Site *leftreg(struct Halfedge *he)
{
  if(he->ELedge == (struct Edge *)NULL) return(bottomsite);
  return( he->ELpm == le ? 
    he->ELedge->reg[le] : he->ELedge->reg[re]);
}

struct Site *rightreg(struct Halfedge *he)
{
  if(he->ELedge == (struct Edge *)NULL) return(bottomsite);
  return( he->ELpm == le ? 
    he->ELedge->reg[re] : he->ELedge->reg[le]);
}

geominit()
{
  struct Edge e;
  float sn;

  freeinit(&efl, sizeof e);
  //nvertices = 0;
  nedges = 0;
  sn = nsites+4;
  sqrt_nsites = sqrt(sn);
  deltay = ymax - ymin;
  deltax = xmax - xmin;
}

struct Edge *bisect(struct  Site *s1, struct  Site *s2)
{
  double dx,dy,adx,ady;
  struct Edge *newedge;

  newedge = (struct Edge *) getfree(&efl);

  newedge->reg[0] = s1;
  newedge->reg[1] = s2;
  ref(s1); 
  ref(s2);
  newedge->ep[0] = (struct Site *) NULL;
  newedge->ep[1] = (struct Site *) NULL;

  dx = s2->coord.x - s1->coord.x;
  dy = s2->coord.y - s1->coord.y;
  adx = dx>0 ? dx : -dx;
  ady = dy>0 ? dy : -dy;
  newedge->c = s1->coord.x * dx + s1->coord.y * dy + (dx*dx + dy*dy)*0.5;
  if (adx>ady)
  {
    newedge->a = 1.0;
    newedge->b = dy/dx;
    newedge->c /= dx;
  }
  else
  {
    newedge->b = 1.0;
    newedge->a = dx/dy;
    newedge->c /= dy;
  }

  newedge->edgenbr = nedges;
  out_bisector(newedge);
  nedges += 1;
  return(newedge);
}

struct Site *intersect(el1, el2, p)
struct Halfedge *el1;
struct Halfedge *el2;
struct Point *p;
{
  struct  Edge *e1, *e2, *e;
  struct  Halfedge *el;
  double d, xint, yint;
  int right_of_site;
  struct Site *v;

  e1 = el1->ELedge;
  e2 = el2->ELedge;
  if(e1 == (struct Edge*)NULL || e2 == (struct Edge*)NULL) 
    return ((struct Site *) NULL);
  if (e1->reg[1] == e2->reg[1])
    return ((struct Site *) NULL);

  d = e1->a * e2->b - e1->b * e2->a;
  /* printf("intersect: d=%g\n", d); */
  if (-1.0e-17<d && d<1.0e-17) 
  {
    return ((struct Site *) NULL);
  }
  
  xint = (e1->c*e2->b - e2->c*e1->b)/d;
  yint = (e2->c*e1->a - e1->c*e2->a)/d;

  if( (e1->reg[1]->coord.y < e2->reg[1]->coord.y) ||
      (e1->reg[1]->coord.y == e2->reg[1]->coord.y &&
    e1->reg[1]->coord.x < e2->reg[1]->coord.x) )
  {
    el = el1;
    e = e1;
  }
  else
  {
    el = el2;
    e = e2;
  }
  right_of_site = xint >= e->reg[1]->coord.x;
  if ((right_of_site && el->ELpm == le) ||
     (!right_of_site && el->ELpm == re)) return ((struct Site *) NULL);

  v = (struct Site *) getfree(&sfl);
  v->refcnt = 0;
  v->coord.x = xint;
  v->coord.y = yint;
  return(v);
}

/* returns 1 if p is to right of halfedge e */
int right_of(struct Halfedge *el, struct Point *p)
{
  struct Edge *e;
  struct Site *topsite;
  int right_of_site, above, fast;
  double dxp, dyp, dxs, t1, t2, t3, yl;

  e = el->ELedge;
  topsite = e->reg[1];
  right_of_site = p->x > topsite->coord.x;
  if(right_of_site && el->ELpm == le) return(1);
  if(!right_of_site && el->ELpm == re) return (0);

  if (e->a == 1.0)
  {
    dyp = p->y - topsite->coord.y;
    dxp = p->x - topsite->coord.x;
    fast = 0;
    if ((!right_of_site &e->b<0.0) | (right_of_site&e->b>=0.0) )
    {
      above = dyp>= e->b*dxp;  
      fast = above;
    }
    else 
    {
      above = p->x + p->y*e->b > e->c;
      if(e->b<0.0) above = !above;
      if (!above) fast = 1;
    }
    if (!fast)
    {
      dxs = topsite->coord.x - (e->reg[0])->coord.x;
      above = e->b * (dxp*dxp - dyp*dyp) < dxs*dyp*(1.0+2.0*dxp/dxs + e->b*e->b);
      if(e->b<0.0) above = !above;
    }
  }
  else  /*e->b==1.0 */
  {
    yl = e->c - e->a*p->x;
    t1 = p->y - yl;
    t2 = p->x - topsite->coord.x;
    t3 = yl - topsite->coord.y;
    above = t1*t1 > t2*t2 + t3*t3;
  }
  return (el->ELpm==le ? above : !above);
}

endpoint(struct Edge *e, int lr, struct Site *s)
{
  e->ep[lr] = s;
  ref(s);
  if(e->ep[re-lr]== (struct Site *) NULL) return;
  out_ep(e);
  deref(e->reg[le]);
  deref(e->reg[re]);
  makefree(e, &efl);
}

float dist(struct Site *s, struct Site *t)
{
  float dx,dy;
  dx = s->coord.x - t->coord.x;
  dy = s->coord.y - t->coord.y;
  return(sqrt(dx*dx + dy*dy));
}

deref(struct  Site *v)
{
  v->refcnt -= 1;
  if (v->refcnt == 0 ) makefree(v, &sfl);
}

ref(struct Site *v)
{
  v->refcnt += 1;
}

PQinsert(struct Halfedge *he, struct Site *v, float offset)
{
  struct Halfedge *last, *next;
  he->vertex = v;
  ref(v);
  he->ystar = v->coord.y + offset;
  last = &PQhash[PQbucket(he)];
  while ((next = last->PQnext) != (struct Halfedge *) NULL &&
        (he->ystar  > next->ystar  ||
        (he->ystar == next->ystar && v->coord.x > next->vertex->coord.x)))
  {
    last = next;
  }
  he->PQnext = last->PQnext; 
  last->PQnext = he;
  PQcount += 1;
}

PQdelete(struct Halfedge *he)
{
  struct Halfedge *last;

  if(he->vertex != (struct Site *) NULL)
  {
    last = &PQhash[PQbucket(he)];
    while (last->PQnext != he) last = last->PQnext;
    last->PQnext = he->PQnext;
    PQcount -= 1;
    deref(he->vertex);
    he->vertex = (struct Site *) NULL;
  }
}

int PQbucket(struct Halfedge *he)
{
  int bucket;

  if  (he->ystar < ymin) bucket = 0;
  else if  (he->ystar >= ymax) bucket = PQhashsize-1;
  else          bucket = (he->ystar - ymin)/deltay * PQhashsize;
  if (bucket<0) bucket = 0;
  if (bucket>=PQhashsize) bucket = PQhashsize-1 ;
  if (bucket < PQmin) PQmin = bucket;
  return(bucket);
}

int PQempty()
{
  return(PQcount==0);
}

struct Point PQ_min()
{
  struct Point answer;
  while(PQhash[PQmin].PQnext == (struct Halfedge *)NULL) {PQmin += 1;}
  answer.x = PQhash[PQmin].PQnext->vertex->coord.x;
  answer.y = PQhash[PQmin].PQnext->ystar;
  return (answer);
}

struct Halfedge *PQextractmin()
{
  struct Halfedge *curr;
  curr = PQhash[PQmin].PQnext;
  PQhash[PQmin].PQnext = curr->PQnext;
  PQcount -= 1;
  return(curr);
}

PQinitialize()
{
  int i;
  struct Point *s;

  PQcount = 0;
  PQmin = 0;
  PQhashsize = 4 * sqrt_nsites;
  PQhash = (struct Halfedge *) myalloc(PQhashsize * sizeof *PQhash);
  for(i=0; i<PQhashsize; i+=1)
    PQhash[i].PQnext = (struct Halfedge *)NULL;
}

/* sort sites on y, then x, coord */
int scomp(s1,s2)
struct Point *s1,*s2;
{
  if(s1->y < s2->y) return(-1);
  if(s1->y > s2->y) return(1);
  if(s1->x < s2->x) return(-1);
  if(s1->x > s2->x) return(1);
  return(0);
}

struct Site *nextone();

//struct Shape ** shapes = NULL;
//int num_shapes = 0;

int convert_to_voronoi(int argc, char **argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int c;
  struct Site *(*next)();

  sorted = 0; triangulate = 0; plot = 0; debug = 0;
  
  freeinit(&sfl, sizeof *sites);
  /*if(sorted)
  {
    scanf("%d %f %f %f %f", &nsites, &xmin, &xmax, &ymin, &ymax);
    next = readone;
  }
  else
  {
    readsites();
    next = nextone;
  }*/
  
  nsites = 0;
  sites = (struct Site *) myalloc(4000*sizeof *sites);
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    int j;
    struct VertexArray * va = &shape->vertex_arrays[0];
    for (j = 0 ; j < shape->num_vertexs ; j++)
    {
      sites[nsites].coord.x = va->vertexs[va->num_dimensions*j+0];
      sites[nsites].coord.y = va->vertexs[va->num_dimensions*j+1];
      sites[nsites].sitenbr = nsites;
    	sites[nsites].refcnt = 0;
    	nsites += 1;
    	if (nsites % 4000 == 0) 
    		sites = (struct Site *) realloc(sites,(nsites+4000)*sizeof*sites);
		}
  	
    //num_shapes++;
    //shapes = realloc(shapes, sizeof(struct Shape*)*num_shapes);
    //shapes[num_shapes-1] = shape;
    free_shape(shape);
  }
  
  int i = 0;
  qsort(sites, nsites, sizeof *sites, scomp);
  xmin=sites[0].coord.x; 
  xmax=sites[0].coord.x;
  for(i=1; i<nsites; i+=1)
  {
    if(sites[i].coord.x < xmin) xmin = sites[i].coord.x;
  	if(sites[i].coord.x > xmax) xmax = sites[i].coord.x;
  }
  ymin = sites[0].coord.y;
  ymax = sites[nsites-1].coord.y;
  
  /*struct VertexArray *va = &shapes[0]->vertex_arrays[0];
  struct Site *s = malloc(sizeof(struct Site));
  s->coord.x = va->vertexs[va->num_dimensions*siteidx+0];
  s->coord.y = va->vertexs[va->num_dimensions*siteidx+1];
  siteidx++;
  return s;*/
  
  siteidx = 0;
  geominit();
  //if(plot) 
  plotinit();
  
  voronoi(0, nextone); // voronoi diagrams
  //voronoi(1, next); // delaunay triangles
  
  exit(0);
}

/* return a single in-storage site */
struct Site *nextone()
{
  for ( ; siteidx < nsites ; siteidx += 1)
  {
    if (siteidx==0 || sites[siteidx].coord.x!=sites[siteidx-1].coord.x 
             || sites[siteidx].coord.y!=sites[siteidx-1].coord.y)
    {
      siteidx += 1;
      return (&sites[siteidx-1]);
    }
  }
  return( (struct Site *)NULL);
}

/* // read all sites, sort, and compute xmin, xmax, ymin, ymax 
readsites()
{ 
  int i;

  nsites=0;
  sites = (struct Site *) myalloc(4000*sizeof *sites);
  while(scanf("%f %f", &sites[nsites].coord.x, &sites[nsites].coord.y)!=EOF)
  {
    sites[nsites].sitenbr = nsites;
    sites[nsites].refcnt = 0;
    nsites += 1;
    if (nsites % 4000 == 0) 
      sites = (struct Site *) realloc(sites,(nsites+4000)*sizeof*sites);
  }
  qsort(sites, nsites, sizeof *sites, scomp);
  xmin=sites[0].coord.x; 
  xmax=sites[0].coord.x;
  for(i=1; i<nsites; i+=1)
  {
    if(sites[i].coord.x < xmin) xmin = sites[i].coord.x;
    if(sites[i].coord.x > xmax) xmax = sites[i].coord.x;
  }
  ymin = sites[0].coord.y;
  ymax = sites[nsites-1].coord.y;
}*/

freeinit(struct  Freelist *fl, int size)
{
  fl->head = (struct Freenode *) NULL;
  fl->nodesize = size;
}

char *getfree(struct  Freelist *fl)
{
  int i;
  struct Freenode *t;
  if(fl->head == (struct Freenode *) NULL)
  {
    t = (struct Freenode *) myalloc(sqrt_nsites * fl->nodesize);
    for(i=0; i<sqrt_nsites; i+=1)   
      makefree((struct Freenode *)((char *)t+i*fl->nodesize), fl);
  }
  t = fl->head;
  fl->head = (fl->head)->nextfree;
  return((char *)t);
}

makefree(struct Freenode *curr, struct Freelist *fl)
{
  curr->nextfree = fl->head;
  fl->head = curr;
}


struct Shape * shape = NULL;

struct Point * vertexes = NULL;
int num_vertexes = 0;

int total_alloc;
char *myalloc(unsigned n)
{
  char *t;
  if ((t=malloc(n)) == (char *) 0)
  {
    fprintf(stderr,"Insufficient memory processing site %d (%d bytes in use)\n",
    siteidx, total_alloc);
    exit(0);
  }
  total_alloc += n;
  return(t);
}

out_bisector(struct Edge *e)
{
  /*if(triangulate & plot & !debug)
    printf("li %g %g %g %g\n", e->reg[0]->coord.x, e->reg[0]->coord.y, 
      e->reg[1]->coord.x, e->reg[1]->coord.y);*/
  //if(!triangulate & !plot & !debug)
  //  printf("l %f %f %f\n", e->a, e->b, e->c);
  /*if(debug)
    printf("line(%d) %gx+%gy=%g, bisecting %d %d\n", e->edgenbr,
      e->a, e->b, e->c, e->reg[le]->sitenbr, e->reg[re]->sitenbr);*/
}

out_ep(struct Edge *e)
{
  fprintf(stderr, "e %d", e->edgenbr);
  fprintf(stderr, " %d ", e->ep[le] != (struct Site *)NULL ? e->ep[le]->sitenbr : -1);
  fprintf(stderr, "%d\n", e->ep[re] != (struct Site *)NULL ? e->ep[re]->sitenbr : -1);
  return;
  int v1 = e->ep[le] != (struct Site *) NULL ? e->ep[le]->sitenbr : -1;
  int v2 = e->ep[re] != (struct Site *) NULL ? e->ep[re]->sitenbr : -1;
  
  if (shape == NULL) shape = new_shape();
  shape->gl_type = GL_LINES;
  struct VertexArray *va = &shape->vertex_arrays[0];
  if (v1 >= 0 && v1 < num_vertexes && v2 >= 0 && v2 < num_vertexes)
  {
    va->vertexs = realloc(va->vertexs, sizeof(double)*va->num_dimensions*(shape->num_vertexs+2));
    va->vertexs[shape->num_vertexs*va->num_dimensions+0] = vertexes[v1].x;
    va->vertexs[shape->num_vertexs*va->num_dimensions+1] = vertexes[v1].y;
    fprintf(stderr, "(%d) %f %f to (%d) %f %f\n", v1, vertexes[v1].x, vertexes[v1].y, v2, vertexes[v2].x, vertexes[v2].y);
    shape->num_vertexs++;
    va->vertexs[shape->num_vertexs*va->num_dimensions+0] = vertexes[v2].x;
    va->vertexs[shape->num_vertexs*va->num_dimensions+1] = vertexes[v2].y;
    shape->num_vertexs++;
  }
  
  /*if(!triangulate & plot) 
    clip_line(e);
  if(!triangulate & !plot)
  {
    printf("e %d", e->edgenbr);
    printf(" %d ", e->ep[le] != (struct Site *)NULL ? e->ep[le]->sitenbr : -1);
    printf("%d\n", e->ep[re] != (struct Site *)NULL ? e->ep[re]->sitenbr : -1);
  }*/
}

out_vertex(struct Site *v)
{
  vertexes = (struct Point *)realloc(vertexes, sizeof(struct Point)*(num_vertexes+1));
  vertexes[num_vertexes].x = v->coord.x;
  vertexes[num_vertexes].y = v->coord.y;
  //fprintf(stderr, "v at %f %f\n", v->coord.x, v->coord.y);
  num_vertexes++;
  /*if(!triangulate & !plot &!debug)
    printf ("v %f %f\n", v->coord.x, v->coord.y);*/
  //if(debug)
  //  printf("vertex(%d) at %f %f\n", v->sitenbr, v->coord.x, v->coord.y);
}

out_site(struct Site *s)
{
  //fprintf(stderr, "s at %f %f\n", s->coord.x, s->coord.y);
  /*if(!triangulate & plot & !debug)
    printf("ci %g %g %g\n", s->coord.x, s->coord.y, cradius);
  if(!triangulate & !plot & !debug)
    printf("s %f %f\n", s->coord.x, s->coord.y);
  if(debug)
    printf("site (%d) at %f %f\n", s->sitenbr, s->coord.x, s->coord.y);*/
}

out_triple(struct Site *s1, struct Site *s2, struct Site *s3)
{
  /*if(triangulate & !plot &!debug)
    printf("%d %d %d\n", s1->sitenbr, s2->sitenbr, s3->sitenbr);
  if(debug)
    printf("circle through left=%d right=%d bottom=%d\n", 
      s1->sitenbr, s2->sitenbr, s3->sitenbr);*/
}

plotinit()
{
  float dx,dy,d;

  dy = ymax - ymin;
  dx = xmax - xmin;
  d = ( dx > dy ? dx : dy) * 1.3;
  pxmin = xmin - (d-dx)/2.0;
  pxmax = xmax + (d-dx)/2.0;
  pymin = ymin - (d-dy)/2.0;
  pymax = ymax + (d-dy)/2.0;
  cradius = (pxmax - pxmin)/350.0;
  //fprintf(stderr, "%g %g %g %g\n", pxmin, pymin, pxmax, pymax);
}

finish_pl()
{
  //if(plot) printf("cl\n");
}

/*int clip_line(struct Edge *e)
{
  fprintf(stderr, "c");
  struct Site *s1, *s2;
  float x1,x2,y1,y2;

  if(e->a == 1.0 && e->b >= 0.0)
  {
    s1 = e->ep[1];
    s2 = e->ep[0];
  }
  else 
  {
    s1 = e->ep[0];
    s2 = e->ep[1];
  }

  if(e->a == 1.0)
  {
    y1 = pymin;
    if (s1 != (struct Site *)NULL && s1->coord.y > pymin)
      y1 = s1->coord.y;
    if (y1 > pymax)
      return;
    x1 = e->c - e->b * y1;
    y2 = pymax;
    if(s2 != (struct Site *)NULL && s2->coord.y < pymax) 
      y2 = s2->coord.y;
    if(y2 < pymin)
      return(0);
    x2 = e->c - e->b * y2;
    if ((x1> pxmax & x2>pxmax) | (x1<pxmin&x2<pxmin)) return;
    if(x1> pxmax)
    {
      x1 = pxmax;
      y1 = (e->c - x1)/e->b;
    }
    if(x1<pxmin)
    {
      x1 = pxmin;
      y1 = (e->c - x1)/e->b;
    }
    if(x2>pxmax)
    {
      x2 = pxmax;
      y2 = (e->c - x2)/e->b;
    }
    if(x2<pxmin)
    {
      x2 = pxmin;
      y2 = (e->c - x2)/e->b;
    }
  }
  else
  {
    x1 = pxmin;
    if (s1!=(struct Site *)NULL && s1->coord.x > pxmin) 
      x1 = s1->coord.x;
    if(x1>pxmax) return(0);
    y1 = e->c - e->a * x1;
    x2 = pxmax;
    if (s2!=(struct Site *)NULL && s2->coord.x < pxmax) 
      x2 = s2->coord.x;
    if(x2<pxmin) return(0);
    y2 = e->c - e->a * x2;
    if ((y1> pymax & y2>pymax) | (y1<pymin&y2<pymin)) return(0);
    if(y1 > pymax)
    {
      y1 = pymax;
      x1 = (e->c - y1)/e->a;
    }
    if(y1 < pymin)
    {
      y1 = pymin;
      x1 = (e->c - y1)/e->a;
    }
    if(y2 > pymax)
    {
      y2 = pymax;
      x2 = (e->c - y2)/e->a;
    }
    if(y2 < pymin)
    {
      y2 = pymin;
      x2 = (e->c - y2)/e->a;
    }
  }
  
  //printf("li %g %g %g %g\n", x1,y1,x2,y2);
}*/



/* implicit parameters: nsites, sqrt_nsites, xmin, xmax, ymin, ymax,
   deltax, deltay (can all be estimates).
   Performance suffers if they are wrong; better to make nsites,
   deltax, and deltay too big than too small.  (?) */

int voronoi(int triangulate, struct Site *(*nextsite)())
{
  struct Site *newsite, *bot, *top, *temp, *p;
  struct Site *v;
  struct Point newintstar;
  int pm;
  struct Halfedge *lbnd, *rbnd, *llbnd, *rrbnd, *bisector;
  struct Edge *e;

  PQinitialize();
  bottomsite = (*nextsite)();
  out_site(bottomsite);
  ELinitialize();

  newsite = (*nextsite)();
  while(1)
  {
    if(!PQempty()) newintstar = PQ_min();

    if (newsite != (struct Site *)NULL 
       && (PQempty() 
       || newsite->coord.y < newintstar.y
        || (newsite->coord.y == newintstar.y 
           && newsite->coord.x < newintstar.x)))
    /* new site is smallest */
    {
      out_site(newsite);
      lbnd = ELleftbnd(&(newsite->coord));
      rbnd = ELright(lbnd);
      bot = rightreg(lbnd);
      e = bisect(bot, newsite);
      bisector = HEcreate(e, le);
      ELinsert(lbnd, bisector);
      if ((p = intersect(lbnd, bisector)) != (struct Site *) NULL) 
      {
        PQdelete(lbnd);
        PQinsert(lbnd, p, dist(p,newsite));
      }
      lbnd = bisector;
      bisector = HEcreate(e, re);
      ELinsert(lbnd, bisector);
      if ((p = intersect(bisector, rbnd)) != (struct Site *) NULL)
      {
        PQinsert(bisector, p, dist(p,newsite));  
      }
      newsite = (*nextsite)();  
    }
    else if (!PQempty()) 
    /* intersection is smallest */
    {
      lbnd = PQextractmin();
      llbnd = ELleft(lbnd);
      rbnd = ELright(lbnd);
      rrbnd = ELright(rbnd);
      bot = leftreg(lbnd);
      top = rightreg(rbnd);
      out_triple(bot, top, rightreg(lbnd));
      v = lbnd->vertex;
      out_vertex(v);
      endpoint(lbnd->ELedge,lbnd->ELpm,v);
      endpoint(rbnd->ELedge,rbnd->ELpm,v);
      ELdelete(lbnd); 
      PQdelete(rbnd);
      ELdelete(rbnd); 
      pm = le;
      if (bot->coord.y > top->coord.y)
      {
        temp = bot;
        bot = top;
        top = temp;
        pm = re;
      }
      e = bisect(bot, top);
      bisector = HEcreate(e, pm);
      ELinsert(llbnd, bisector);
      endpoint(e, re-pm, v);
      deref(v);
      if((p = intersect(llbnd, bisector)) != (struct Site *) NULL)
      {
        PQdelete(llbnd);
        PQinsert(llbnd, p, dist(p,bot));
      }
      if ((p = intersect(bisector, rrbnd)) != (struct Site *) NULL)
      {
        PQinsert(bisector, p, dist(p,bot));
      }
    }
    else break;
  }

  for (lbnd = ELright(ELleftend) ; lbnd != ELrightend ; lbnd = ELright(lbnd))
  {
    e = lbnd->ELedge;
    out_ep(e);
  }
  finish_pl();
  
  if (shape != NULL && shape->vertex_arrays != NULL && shape->vertex_arrays[0].vertexs != NULL)
  {
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}

