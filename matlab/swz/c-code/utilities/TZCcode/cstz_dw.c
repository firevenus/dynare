#include "cstz.h"

#include <float.h>
#include <string.h>    /*  For memmove, etc.   ansi-c*/
#include "mathlib.h"

#include "modify_for_mex.h"

/*  //????????   ansi-c*/
/*  //------- For computing inverse Hessian only. -------   ansi-c*/
/*  //static struct TStateModel_tag *SetModelGlobalForCovariance(struct TStateModel_tag *smodel_ps);   ansi-c*/
/*  //static double ObjFuncForSmodel(double *x0_p, int d_x0);   ansi-c*/
/*  //static double opt_logOverallPosteriorKernal(struct TStateModel_tag *smodel_ps, TSdvector *xchange_dv);   ansi-c*/

static double logCondPostKernTimet(double *xchange_p, int t, struct TStateModel_tag *smodel_ps);
static double neglogPostKern_hess(double *xchange_pd, struct TStateModel_tag *smodel_ps);
static void hesscd_smodel(TSdmatrix *H_dm, TSdvector *x_dv, struct TStateModel_tag *smodel_ps, double (*fcn)(double *x, struct TStateModel_tag *), double grdh, double f0);

TSdp2m5 *CreateP2m5(const double p, const double bound)
{
   TSdp2m5 *x_dp2m5 = tzMalloc(1, TSdp2m5);

   if (p<=0.0 && p>=1.0)  fn_DisplayError(".../cstz.c/CreateP2m5():  Input probability p must be between 0.0 and 1.0");
   if ((x_dp2m5->bound=bound)<=0.0)  fn_DisplayError(".../cstz.c/CreateP2m5():  Real bound must be positive");

   x_dp2m5->cnt = 0;
   x_dp2m5->ndeg = 0;
   x_dp2m5->p = tzMalloc(5, double);
   x_dp2m5->q = tzMalloc(5, double);
   x_dp2m5->m = tzMalloc(5, int);

/*     //=== 5 markers.   ansi-c*/
   x_dp2m5->p[0] = 0.00;
   x_dp2m5->p[1] = 0.5*p;
   x_dp2m5->p[2] = p;
   x_dp2m5->p[3] = 0.5*(1.0+p);
   x_dp2m5->p[4] = 1.00;
/*     //=== Now 9 markers.   ansi-c*/
/*     // x_dp2m5->p[0] = 0.00;   ansi-c*/
/*     // x_dp2m5->p[1] = 0.25*p   ansi-c*/
/*     // x_dp2m5->p[2] = 0.5*p;   ansi-c*/
/*     // x_dp2m5->p[3] = 0.75*p;   ansi-c*/
/*     // x_dp2m5->p[4] = p;   ansi-c*/
/*     // x_dp2m5->p[5] = 0.25 + 0.75*p;   ansi-c*/
/*     // x_dp2m5->p[6] = 0.5*(1.0+p);   ansi-c*/
/*     // x_dp2m5->p[7] = 0.75 + 0.25*p;   ansi-c*/
/*     // x_dp2m5->p[8] = 1.00;   ansi-c*/

   return (x_dp2m5);
}
TSdp2m5 *DestroyP2m5(TSdp2m5 *x_dp2m5)
{
   if (x_dp2m5) {
      swzFree(x_dp2m5->m);
      swzFree(x_dp2m5->q);
      swzFree(x_dp2m5->p);

      swzFree(x_dp2m5);
      return ((TSdp2m5 *)NULL);
   }
   else  return (x_dp2m5);
}
TSdvectorp2m5 *CreateVectorP2m5(const int n, const double p, const double bound)
{
   int _i;
/*     //   ansi-c*/
   TSdvectorp2m5 *x_dvp2m5 = tzMalloc(1, TSdvectorp2m5);

   x_dvp2m5->n = n;
   x_dvp2m5->v = tzMalloc(n, TSdp2m5 *);
   for (_i=n-1; _i>=0; _i--)
      x_dvp2m5->v[_i] = CreateP2m5(p, bound);

   return (x_dvp2m5);
}
TSdvectorp2m5 *DestroyVectorP2m5(TSdvectorp2m5 *x_dvp2m5)
{
   int _i;

   if (x_dvp2m5) {
      for (_i=x_dvp2m5->n-1; _i>=0; _i--)
         x_dvp2m5->v[_i] = DestroyP2m5(x_dvp2m5->v[_i]);
      swzFree(x_dvp2m5->v);

      swzFree(x_dvp2m5);
      return ((TSdvectorp2m5 *)NULL);
   }
   else  return (x_dvp2m5);
}
TSdmatrixp2m5 *CreateMatrixP2m5(const int nrows, const int ncols, const double p, const double bound)
{
   int _i;
/*     //   ansi-c*/
   TSdmatrixp2m5 *X_dmp2m5 = tzMalloc(1, TSdmatrixp2m5);

   X_dmp2m5->nrows = nrows;
   X_dmp2m5->ncols = ncols;
   X_dmp2m5->M = tzMalloc(nrows*ncols, TSdp2m5 *);
   for (_i=nrows*ncols-1; _i>=0; _i--)
      X_dmp2m5->M[_i] = CreateP2m5(p, bound);

   return (X_dmp2m5);
}
TSdmatrixp2m5 *DestroyMatrixP2m5(TSdmatrixp2m5 *X_dmp2m5)
{
   int _i;

   if (X_dmp2m5) {
      for (_i=X_dmp2m5->nrows*X_dmp2m5->ncols-1; _i>=0; _i--)
         X_dmp2m5->M[_i] = DestroyP2m5(X_dmp2m5->M[_i]);
      swzFree(X_dmp2m5->M);

      swzFree(X_dmp2m5);
      return ((TSdmatrixp2m5 *)NULL);
   }
   else  return (X_dmp2m5);
}
TSdcellp2m5 *CreateCellP2m5(const TSivector *rows_iv, const TSivector *cols_iv, const double p, const double bound)
{
   int _i;
   int ncells;
/*     //   ansi-c*/
   TSdcellp2m5 *X_dcp2m5 = tzMalloc(1, TSdcellp2m5);


   if (!rows_iv || !cols_iv || !rows_iv->flag || !cols_iv->flag)  fn_DisplayError(".../cstz.c/CreateCellP2m5(): Input row and column vectors must be (1) created and (2) assigned legal values");
   if ((ncells=rows_iv->n) != cols_iv->n)  fn_DisplayError(".../cstz.c/CreateCellP2m5(): Length of rows_iv must be the same as that of cols_iv");


   X_dcp2m5->ncells = ncells;
   X_dcp2m5->C = tzMalloc(ncells, TSdmatrixp2m5 *);
   for (_i=ncells-1; _i>=0; _i--)
      X_dcp2m5->C[_i] = CreateMatrixP2m5(rows_iv->v[_i], cols_iv->v[_i], p, bound);

   return (X_dcp2m5);
}
TSdcellp2m5 *DestroyCellP2m5(TSdcellp2m5 *X_dcp2m5)
{
   int _i;

   if (X_dcp2m5) {
      for (_i=X_dcp2m5->ncells-1; _i>=0; _i--)
         X_dcp2m5->C[_i] = DestroyMatrixP2m5(X_dcp2m5->C[_i]);
      swzFree(X_dcp2m5->C);

      swzFree(X_dcp2m5);
      return ((TSdcellp2m5 *)NULL);
   }
   else  return (X_dcp2m5);
}
TSdfourthp2m5 *CreateFourthP2m5(const int ndims, const TSivector *rows_iv, const TSivector *cols_iv, const double p, const double bound)
{
   int _i;
/*     //   ansi-c*/
   TSdfourthp2m5 *X_d4p2m5 = tzMalloc(1, TSdfourthp2m5);


   if (!rows_iv || !cols_iv || !rows_iv->flag || !cols_iv->flag)  fn_DisplayError(".../cstz.c/CreateFourthP2m5(): Input row and column vectors must be (1) created and (2) assigned legal values");
   if (rows_iv->n != cols_iv->n)  fn_DisplayError(".../cstz.c/CreateFourthP2m5(): Length of rows_iv must be the same as that of cols_iv");


   X_d4p2m5->ndims = ndims;
   X_d4p2m5->F = tzMalloc(ndims, TSdcellp2m5 *);
   for (_i=ndims-1; _i>=0; _i--)
      X_d4p2m5->F[_i] = CreateCellP2m5(rows_iv, cols_iv, p, bound);

   return (X_d4p2m5);
}
TSdfourthp2m5 *DestroyFourthP2m5(TSdfourthp2m5 *X_d4p2m5)
{
   int _i;

   if (X_d4p2m5) {
      for (_i=X_d4p2m5->ndims-1; _i>=0; _i--)
         X_d4p2m5->F[_i] = DestroyCellP2m5(X_d4p2m5->F[_i]);
      swzFree(X_d4p2m5->F);

      swzFree(X_d4p2m5);
      return ((TSdfourthp2m5 *)NULL);
   }
   else  return (X_d4p2m5);
}



int P2m5Update(TSdp2m5 *x_dp2m5, const double newval)
{
/*     //5-marker P2 algorithm.   ansi-c*/
/*     //quantiles q[0] to q[4] correspond to 5-marker probabilities {0.0, p/5, p, (1+p)/5, 1.0}.   ansi-c*/
/*     //Outputs:   ansi-c*/
/*     //  x_dp2m5->q, the markers x_dp2m5->m, is updated and only x_dp2m5->q[2] is used.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  newval: new random number.   ansi-c*/
/*     //   ansi-c*/
/*     // January 2003.   ansi-c*/
   int k, j;
   double a;
   double qm, dq;
   int i, dm, dn;


   if (!x_dp2m5)  fn_DisplayError(".../cstz.c/P2m5Update(): x_dp2m5 must be created");

/*     //if (isgreater(newval, -P2REALBOUND) && isless(newval, P2REALBOUND)) {   ansi-c*/
   if (isfinite(newval) && newval > -x_dp2m5->bound && newval < x_dp2m5->bound) {
      if (++x_dp2m5->cnt > 5) {
/*           //Updating the quantiles and markers.   ansi-c*/
         for (i=0; x_dp2m5->q[i]<=newval && i<5; i++) ;
         if (i==0) { x_dp2m5->q[0]=newval; i++; }
         if (i==5) { x_dp2m5->q[4]=newval; i--; }
         for (; i<5; i++) x_dp2m5->m[i]++;
         for (i=1; i<4; i++) {
            dq = x_dp2m5->p[i]*x_dp2m5->m[4];
            if (x_dp2m5->m[i]+1<=dq && (dm=x_dp2m5->m[i+1]-x_dp2m5->m[i])>1) {
               dn = x_dp2m5->m[i]-x_dp2m5->m[i-1];
               dq = ((dn+1)*(qm=x_dp2m5->q[i+1]-x_dp2m5->q[i])/dm+
                  (dm-1)*(x_dp2m5->q[i]-x_dp2m5->q[i-1])/dn)/(dm+dn);
               if (qm<dq) dq = qm/dm;
               x_dp2m5->q[i] += dq;
               x_dp2m5->m[i]++;
            } else
            if (x_dp2m5->m[i]-1>=dq && (dm=x_dp2m5->m[i]-x_dp2m5->m[i-1])>1) {
               dn = x_dp2m5->m[i+1]-x_dp2m5->m[i];
               dq = ((dn+1)*(qm=x_dp2m5->q[i]-x_dp2m5->q[i-1])/dm+
                  (dm-1)*(x_dp2m5->q[i+1]-x_dp2m5->q[i])/dn)/(dm+dn);
               if (qm<dq) dq = qm/dm;
               x_dp2m5->q[i] -= dq;
               x_dp2m5->m[i]--;
            }
         }
      }
      else if (x_dp2m5->cnt < 5) {
/*           //Fills the initial values.   ansi-c*/
         x_dp2m5->q[x_dp2m5->cnt-1] = newval;
         x_dp2m5->m[x_dp2m5->cnt-1] = x_dp2m5->cnt-1;
      }
      else {
/*           //=== Last filling of initial values.   ansi-c*/
         x_dp2m5->q[4] = newval;
         x_dp2m5->m[4] = 4;
/*           //=== P2 algorithm begins with reshuffling quantiles and makers.   ansi-c*/
         for (j=1; j<5; j++) {
            a = x_dp2m5->q[j];
            for (k=j-1; k>=0 && x_dp2m5->q[k]>a; k--)
               x_dp2m5->q[k+1] = x_dp2m5->q[k];
            x_dp2m5->q[k+1]=a;
         }
      }
   }
   else  ++x_dp2m5->ndeg;   /*  Throwing away the draws to treat exceptions.   ansi-c*/

   return (x_dp2m5->cnt);
}

void P2m5VectorUpdate(TSdvectorp2m5 *x_dvp2m5, const TSdvector *newval_dv)
{
   int _i, _n;

   if (!x_dvp2m5 || !newval_dv || !newval_dv->flag)  fn_DisplayError(".../cstz.c/P2m5VectorUpdate():  (1) Vector struct x_dvp2m5 must be created and (2) input new value vector must be crated and given legal values");
   if ((_n=newval_dv->n) != x_dvp2m5->n)
      fn_DisplayError(".../cstz.c/P2m5VectorUpdate(): dimension of x_dvp2m5 must match that of newval_dv");

   for (_i=_n-1; _i>=0; _i--)
      P2m5Update(x_dvp2m5->v[_i], newval_dv->v[_i]);
}

void P2m5MatrixUpdate(TSdmatrixp2m5 *X_dmp2m5, const TSdmatrix *newval_dm)
{
   int _i;
   int nrows, ncols;

   if (!X_dmp2m5 || !newval_dm || !newval_dm->flag)  fn_DisplayError(".../cstz.c/P2m5MatrixUpdate():  (1) Matrix struct X_dmp2m5 must be created and (2) input new value matrix must be crated and given legal values");
   if ((nrows=newval_dm->nrows) != X_dmp2m5->nrows || (ncols=newval_dm->ncols) != X_dmp2m5->ncols)
      fn_DisplayError(".../cstz.c/P2m5MatrixUpdate(): Number of rows and colums in X_dmp2m5 must match those of newval_dm");

   for (_i=nrows*ncols-1; _i>=0; _i--)
      P2m5Update(X_dmp2m5->M[_i], newval_dm->M[_i]);
}

void P2m5CellUpdate(TSdcellp2m5 *X_dcp2m5, const TSdcell *newval_dc)
{
   int _i;
   int ncells;

   if (!X_dcp2m5 || !newval_dc)  fn_DisplayError(".../cstz.c/P2m5CellUpdate():  (1) Cell struct X_dcp2m5 must be created and (2) input new value cell must be crated and given legal values");
   if ((ncells=newval_dc->ncells) != X_dcp2m5->ncells)
      fn_DisplayError(".../cstz.c/P2m5MatrixUpdate(): Number of cells in X_dcp2m5 must match that of newval_dc");

   for (_i=ncells-1; _i>=0; _i--)
      P2m5MatrixUpdate(X_dcp2m5->C[_i], newval_dc->C[_i]);
}

void P2m5FourthUpdate(TSdfourthp2m5 *X_d4p2m5, const TSdfourth *newval_d4)
{
   int _i;
   int ndims;

   if (!X_d4p2m5 || !newval_d4)  fn_DisplayError(".../cstz.c/P2m5FourthUpdate():  (1) Fourth struct X_d4p2m5 must be created and (2) input new value fourth must be crated and given legal values");
   if ((ndims=newval_d4->ndims) != X_d4p2m5->ndims)
      fn_DisplayError(".../cstz.c/P2m5FourthUpdate(): Number of fourths in X_d4p2m5 must match that of newval_d4");

   for (_i=ndims-1; _i>=0; _i--)
      P2m5CellUpdate(X_d4p2m5->F[_i], newval_d4->F[_i]);
}




/*  //---------------------------------------------------------------------   ansi-c*/
/*  //---------------------------------------------------------------------   ansi-c*/
#if defined( CSMINWEL_OPTIMIZATION )
   #define STPS 6.0554544523933391e-6    /* step size = pow(DBL_EPSILON,1.0/3) */
   void fn_gradcd(double *g, double *x, int n, double grdh,
                  double (*fcn)(double *x, int n, double **args, int *dims),
                  double **args, int *dims) {
/*        //Outputs:   ansi-c*/
/*        //  g: the gradient n-by-1 g (no need to be initialized).   ansi-c*/
/*        //Inputs:   ansi-c*/
/*        //  grdh: step size.  If ==0.0, then dh is set automatically; otherwise, grdh is taken as a step size, often set as 1.0e-004.   ansi-c*/
/*        //  x:  no change in the end although will be added or substracted by dh during the function (but in the end the original value will be put back).   ansi-c*/

      double dh, fp, fm, tmp, *xp;
      int i;
      for (i=0, xp=x; i<n; i++, xp++, g++) {
         dh = grdh?grdh:(fabs(*xp)<1?STPS:STPS*(*xp));
         tmp = *xp;
         *xp += dh;
         dh = *xp - tmp;                    /*   This increases the precision slightly.   ansi-c*/
         fp = fcn(x,n,args,dims);
         *xp = tmp - dh;
         fm = fcn(x,n,args,dims);
         *g = (fp-fm)/(2*dh);
         *xp = tmp;                         /*   Put the original value of x[i] back to x[i] so that the content x[i] is still unaltered.   ansi-c*/
      }
   }
   #undef STPS

   #define STPS 6.0554544523933391e-6    /* step size = pow(DBL_EPSILON,1.0/3) */
   void fn_hesscd(double *H, double *x, int n, double grdh,
                  double (*fcn)(double *x, int n, double **args, int *dims),
                  double **args, int *dims) {
      double dhi, dhj, f1, f2, f3, f4, tmpi, tmpj, *xpi, *xpj;
      int i, j;
      for (i=0, xpi=x; i<n; i++, xpi++) {
         dhi = grdh?grdh:(fabs(*xpi)<1?STPS:STPS*(*xpi));
         tmpi = *xpi;
         for (j=i, xpj=x+i; j<n; j++, xpj++)
            if (i==j) {
               /* f2 = f3 when i = j */
               f2 = fcn(x,n,args,dims);

               /* this increases precision slightly */
               *xpi += dhi;
               dhi = *xpi - tmpi;

               /* calculate f1 and f4 */
               *xpi = tmpi + 2*dhi;
               f1 = fcn(x,n,args,dims);
               *xpi = tmpi - 2*dhi;
               f4 = fcn(x,n,args,dims);

               /* diagonal element */
               H[i*(n+1)] = (f1-2*f2+f4)/(4*dhi*dhi);

               /* reset to intial value */
               *xpi = tmpi;
            } else {
               dhj = grdh?grdh:(fabs(*xpj)<1?STPS:STPS*(*xpj));
               tmpj = *xpj;

               /* this increases precision slightly */
               *xpi += dhi;
               dhi = *xpi - tmpi;
               *xpj += dhj;
               dhj = *xpj - tmpj;

               /* calculate f1, f2, f3 and f4 */
               *xpj = tmpj + dhj;
               f1 = fcn(x,n,args,dims);
               *xpi = tmpi - dhi;
               f2 = fcn(x,n,args,dims);
               *xpi = tmpi + dhi;
               *xpj = tmpj - dhj;
               f3 = fcn(x,n,args,dims);
               *xpi = tmpi - dhi;
               f4 = fcn(x,n,args,dims);

               /* symmetric elements */
               H[i+j*n] = H[j+i*n] = (f1-f2-f3+f4)/(4*dhi*dhj);

               /* reset to intial values */
               *xpi = tmpi;
               *xpj = tmpj;
            }
      }
   }
   #undef STPS
#elif defined( IMSL_OPTIMIZATION )
   #define STPS 6.0554544523933391e-6    /* step size = pow(DBL_EPSILON,1.0/3) */
   void fn_gradcd(double *g, double *x, int n, double grdh,
                  double fcn(int n, double *x)  /*   IMSL   ansi-c*/
/*                    //void NAG_CALL fcn(Integer n,double x[],double *f,double g[],Nag_Comm *comm)   ansi-c*/
                  ) {
/*        //Outputs:   ansi-c*/
/*        //  g: the gradient n-by-1 g (no need to be initialized).   ansi-c*/
/*        //Inputs:   ansi-c*/
/*        //  grdh: step size.  If ==0.0, then dh is set automatically; otherwise, grdh is taken as a step size, often set as 1.0e-004.   ansi-c*/
/*        //  x:  no change in the end although will be added or substracted by dh during the function (but in the end the original value will be put back).   ansi-c*/

      double dh, fp, fm, tmp, *xp;
      int i;
      for (i=0, xp=x; i<n; i++, xp++, g++) {
         dh = grdh?grdh:(fabs(*xp)<1?STPS:STPS*(*xp));
         tmp = *xp;
         *xp += dh;
         dh = *xp - tmp;                    /*   This increases the precision slightly.   ansi-c*/
         fp = fcn(n,x);  /*   IMSL   ansi-c*/
         //fcn(n,x,&fp,NULL,NULL); /* NAG */
         *xp = tmp - dh;
         fm = fcn(n,x);  /*   IMSL   ansi-c*/
/*           //fcn(n,x,&fm,NULL,NULL);   ansi-c*/
         *g = (fp-fm)/(2*dh);
         *xp = tmp;                         /*   Put the original value of x[i] back to x[i] so that the content x[i] is still unaltered.   ansi-c*/
      }
   }
   #undef STPS

   #define STPS 6.0554544523933391e-6    /* step size = pow(DBL_EPSILON,1.0/3) */
   void fn_hesscd(double *H, double *x, int n, double grdh,
                  double fcn(int n, double *x)  /*   IMSL   ansi-c*/
/*                    //void NAG_CALL fcn(Integer n,double x[],double *f,double g[],Nag_Comm *comm)   ansi-c*/
                  ) {
      double dhi, dhj, f1, f2, f3, f4, tmpi, tmpj, *xpi, *xpj;
      int i, j;
      for (i=0, xpi=x; i<n; i++, xpi++) {
         dhi = grdh?grdh:(fabs(*xpi)<1?STPS:STPS*(*xpi));
         tmpi = *xpi;
         for (j=i, xpj=x+i; j<n; j++, xpj++)
            if (i==j) {
               /* f2 = f3 when i = j */
               f2 = fcn(n,x);  /*   IMSL   ansi-c*/
/*                 //fcn(n,x,&f2,NULL,NULL);   ansi-c*/

               /* this increases precision slightly */
               *xpi += dhi;
               dhi = *xpi - tmpi;

               /* calculate f1 and f4 */
               *xpi = tmpi + 2*dhi;
               f1 = fcn(n,x);  /*   IMSL   ansi-c*/
/*                 //fcn(n,x,&f1,NULL,NULL);   ansi-c*/
               *xpi = tmpi - 2*dhi;
               f4 = fcn(n,x); /* IMSL */
/*                 //fcn(n,x,&f4,NULL,NULL);   ansi-c*/

               /* diagonal element */
               H[i*(n+1)] = (f1-2*f2+f4)/(4*dhi*dhi);

               /* reset to intial value */
               *xpi = tmpi;
            } else {
               dhj = grdh?grdh:(fabs(*xpj)<1?STPS:STPS*(*xpj));
               tmpj = *xpj;

               /* this increases precision slightly */
               *xpi += dhi;
               dhi = *xpi - tmpi;
               *xpj += dhj;
               dhj = *xpj - tmpj;

               /* calculate f1, f2, f3 and f4 */
               *xpj = tmpj + dhj;
               f1 = fcn(n,x);  /*   IMSL   ansi-c*/
/*                 //fcn(n,x,&f1,NULL,NULL);   ansi-c*/
               *xpi = tmpi - dhi;
               f2 = fcn(n,x);  /*   IMSL   ansi-c*/
/*                 //fcn(n,x,&f2,NULL,NULL);   ansi-c*/
               *xpi = tmpi + dhi;
               *xpj = tmpj - dhj;
               f3 = fcn(n,x);  /*   IMSL   ansi-c*/
/*                 //fcn(n,x,&f3,NULL,NULL);   ansi-c*/
               *xpi = tmpi - dhi;
               f4 = fcn(n,x);  /*   IMSL   ansi-c*/
/*                 //fcn(n,x,&f4,NULL,NULL);   ansi-c*/

               /* symmetric elements */
               H[i+j*n] = H[j+i*n] = (f1-f2-f3+f4)/(4*dhi*dhj);

               /* reset to intial values */
               *xpi = tmpi;
               *xpj = tmpj;
            }
      }
   }
   #undef STPS
#endif



/*  //-------------------------------   ansi-c*/
/*  //Modified from fn_gradcd() in cstz.c for the conjugate gradient method I or II   ansi-c*/
/*  //-------------------------------   ansi-c*/
#define STPS 1.0e-04     /*   6.0554544523933391e-6 step size = pow(DBL_EPSILON,1.0/3)   ansi-c*/
#define GRADMANUAL  1.0e+01    /*  Arbitrarily (manually) set gradient.   ansi-c*/
void gradcd_gen(double *g, double *x, int n, double (*fcn)(double *x, int n), double *grdh, double f0) {
/*     //Outputs:   ansi-c*/
/*     //  g: the gradient n-by-1 g (no need to be initialized).   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  x: the vector point at which the gradient is evaluated.  No change in the end although will be added or substracted by dh during the function (but in the end the original value will be put back).   ansi-c*/
/*     //  n: the dimension of g or x.   ansi-c*/
/*     //  fcn(): the function for which the gradient is evaluated   ansi-c*/
/*     //  grdh: step size.  If NULL, then dh is set automatically; otherwise, grdh is taken as a step size, often set as 1.0e-004.   ansi-c*/
/*     //  f0: the value of (*fcn)(x).   NOT used in this function except dealing with the boundary (NEARINFINITY) for the   ansi-c*/
/*     //    minimization problem, but to be compatible with a genral function call where, say, gradfw_gen() and cubic   ansi-c*/
/*     //    interpolation of central difference method will use f0.   ansi-c*/

   double dh, dhi, dh2i, fp, fm, tmp, *xp;
   int i;

   if (grdh) {
/*        //=== If f0 >= NEARINFINITY, we're in a bad region and so we assume it's flat in this bad region. This assumption may or may not work for a third-party optimimization routine.   ansi-c*/
      if (f0 >= NEARINFINITY)
      {
         for (i=n-1; i>=0; i--)
            g[i] = GRADMANUAL;
         return;;    /*  Early exit.   ansi-c*/
      }

      dh2i = (dhi=1.0/(dh=*grdh))/2.0;
      for (i=0, xp=x; i<n; i++, xp++, g++) {
         tmp = *xp;
         *xp += dh;
/*           //The following statement is bad because dh does not get reset at the beginning of the loop and thus may get changed continually within the loop.   ansi-c*/
/*           //  dh = *xp - tmp;                   // This increases the precision slightly.   ansi-c*/
         fp = fcn(x, n);  /*  For frprmn() CGI_OPTIMIZATION   ansi-c*/
/*           //fp = fcn(n,x); // IMSL   ansi-c*/
/*         //fcn(n,x,&fp,NULL,NULL); /* NAG */
         *xp = tmp - dh;
         fm = fcn(x, n);  /*  For frprmn() CGI_OPTIMIZATION   ansi-c*/
/*           //fm = fcn(n,x); // IMSL   ansi-c*/
/*           //fcn(n,x,&fm,NULL,NULL);   ansi-c*/

/*           //=== Checking the boundary condition for the minimization problem.   ansi-c*/
         if ((fp < NEARINFINITY) && (fm < NEARINFINITY))  *g = (fp-fm)*dh2i;
         else if (fp < NEARINFINITY)  *g = (fp-f0)*dhi;
         else if (fm < NEARINFINITY)  *g = (f0-fm)*dhi;
         else  *g = GRADMANUAL;

         *xp = tmp;                         /*   Put the original value of x[i] back to x[i] so that the content x[i] is still unaltered.   ansi-c*/
      }

   }
   else {
/*        //=== If f0 >= NEARINFINITY, we're in a bad region and so we assume it's flat in this bad region. This assumption may or may not work for a third-party optimimization routine.   ansi-c*/
      if (f0 >= NEARINFINITY)
      {
         for (i=n-1; i>=0; i--)
            g[i] = GRADMANUAL;
         return;;    /*  Early exit.   ansi-c*/
      }

      for (i=0, xp=x; i<n; i++, xp++, g++) {
         dh = fabs(*xp)<=1 ? STPS : STPS*(*xp);
         tmp = *xp;
         *xp += dh;
         dh = *xp - tmp;                    /*   This increases the precision slightly.   ansi-c*/
         fp = fcn(x, n);    /*  For frprmn() CGI_OPTIMIZATION   ansi-c*/
/*           //fp = fcn(n,x); // IMSL   ansi-c*/
/*         //fcn(n,x,&fp,NULL,NULL); /* NAG */
         *xp = tmp - dh;
         fm = fcn(x, n);  /*  For frprmn() CGI_OPTIMIZATION   ansi-c*/
/*           //fm = fcn(n,x); // IMSL   ansi-c*/
/*           //fcn(n,x,&fm,NULL,NULL);   ansi-c*/

/*           //=== Checking the boundary condition for the minimization problem.   ansi-c*/
         if ((fp < 0.5*NEARINFINITY) && (fm < 0.5*NEARINFINITY))  *g = (fp-fm)/(2.0*dh);
         else if (fp < 0.5*NEARINFINITY)  *g = (fp-f0)/dh;
         else if (fm < 0.5*NEARINFINITY)  *g = (f0-fm)/dh;
         else  *g = GRADMANUAL;

         *xp = tmp;                         /*   Put the original value of x[i] back to x[i] so that the content x[i] is still unaltered.   ansi-c*/
      }
   }
}
#undef STPS
#undef GRADMANUAL


/*  //-------------------------------   ansi-c*/
/*  //Forward difference gradient: much faster than gradcd_gen() when the objective function is very expensive to evaluate.   ansi-c*/
/*  //-------------------------------   ansi-c*/
#define STPS 1.0e-04     /*   6.0554544523933391e-6 step size = pow(DBL_EPSILON,1.0/3)   ansi-c*/
void gradfd_gen(double *g, double *x, int n, double (*fcn)(double *x, int n), double *grdh, double f0) {
/*     //Outputs:   ansi-c*/
/*     //  g: the gradient n-by-1 g (no need to be initialized).   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  x: the vector point at which the gradient is evaluated.  No change in the end although will be added or substracted by dh during the function (but in the end the original value will be put back).   ansi-c*/
/*     //  n: the dimension of g or x.   ansi-c*/
/*     //  fcn(): the function for which the gradient is evaluated   ansi-c*/
/*     //  grdh: step size.  If NULL, then dh is set automatically; otherwise, grdh is taken as a step size, often set as 1.0e-004.   ansi-c*/
/*     //  f0: the value of (*fcn)(x).   NOT used in this function except dealing with the boundary (NEARINFINITY) for the   ansi-c*/
/*     //    minimization problem, but to be compatible with a genral function call where, say, gradfw_gen() and cubic   ansi-c*/
/*     //    interpolation of central difference method will use f0.   ansi-c*/

   double dh, dhi, fp, tmp, *xp;
   int i;
   if (grdh) {
      dhi = 1.0/(dh=*grdh);
      for (i=0, xp=x; i<n; i++, xp++, g++) {
         dh = fabs(*xp)<=1 ? STPS : STPS*(*xp);
         tmp = *xp;
         *xp += dh;
         if ( (fp=fcn(x, n)) < NEARINFINITY )  *g = (fp-f0)*dhi;    /*  For frprmn() CGI_OPTIMIZATION   ansi-c*/
         else {
/*              //Switches to the other side of the boundary.   ansi-c*/
            *xp = tmp - dh;
            *g = (f0-fcn(x,n))*dhi;
         }
         *xp = tmp;                         /*   Put the original value of x[i] back to x[i] so that the content x[i] is still unaltered.   ansi-c*/
      }

   }
   else {
      for (i=0, xp=x; i<n; i++, xp++, g++) {
         dh = fabs(*xp)<=1 ? STPS : STPS*(*xp);
         tmp = *xp;
         *xp += dh;
         dh = *xp - tmp;                    /*   This increases the precision slightly.   ansi-c*/
         if ( (fp=fcn(x, n)) < NEARINFINITY )  *g = (fp-f0)/dh;    /*  For frprmn() CGI_OPTIMIZATION   ansi-c*/
         else {
/*              //Switches to the other side of the boundary.   ansi-c*/
            *xp = tmp - dh;
            *g = (f0-fcn(x,n))/dh;
         }

         *xp = tmp;                         /*   Put the original value of x[i] back to x[i] so that the content x[i] is still unaltered.   ansi-c*/
      }
   }
}
#undef STPS



/*  //====================================================================================================   ansi-c*/
/*  //= Central difference gradient for logLH at time t, using DW's smodel.   ansi-c*/
/*  //====================================================================================================   ansi-c*/
#define STPS 1.0e-04     /*   6.0554544523933391e-6 step size = pow(DBL_EPSILON,1.0/3)   ansi-c*/
#define GRADMANUAL  1.0e+01    /*  Arbitrarily (manually) set gradient.   ansi-c*/
void gradcd_timet(TSdvector *g_dv, TSdvector *x_dv, int t, struct TStateModel_tag *smodel_ps, double (*fcn)(double *x, int t, struct TStateModel_tag *smodel_ps), double grdh, double f0)
{
/*     //Outputs:   ansi-c*/
/*     //  g_dv: the gradient n-by-1 g (no need to be initialized).   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  x_dv: the vector point at which the gradient is evaluated.  No change in the end although will be added or substracted by dh during the function (but in the end the original value will be put back).   ansi-c*/
/*     //  fcn(): the log LH or posterior function for which the gradient is evaluated   ansi-c*/
/*     //  grdh: step size.  If 0.0, then dh is set automatically; otherwise, grdh is taken as a step size, often set as 1.0e-004.   ansi-c*/
/*     //  f0: the value of (*fcn)(x).   NOT used in this function except dealing with the boundary (NEARINFINITY) for the   ansi-c*/
/*     //    minimization problem, but to be compatible with a genral function call where, say, gradfw_gen() and cubic   ansi-c*/
/*     //    interpolation of central difference method will use f0.   ansi-c*/

   double dh, dhi, dh2i, fp, fm, tmp, *xp;
   int i;
/*     //--- Accessible variables.   ansi-c*/
   int n;
   double *g, *x;

   if (!g_dv)  fn_DisplayError(".../cstz.c/gradcd_timet(): the input g_dv must be allocated memory");
   if (!x_dv)  fn_DisplayError(".../cstz.c/gradcd_timet(): the input x_dv must be allocated memory");
   if (!x_dv->flag)  fn_DisplayError(".../cstz.c/gradcd_timet(): the input x_dv must be given legal values");
   if ((n=g_dv->n) != x_dv->n)  fn_DisplayError(".../cstz.c/gradcd_timet(): dimensions of g_dv and x_dv must be the same");

   g = g_dv->v;
   x = x_dv->v;

   if (grdh>0.0)
   {
/*        //=== If f0 <= -0.5*NEARINFINITY, we're in a bad region and so we assume it's GRADMANUAL in this bad region. This assumption may or may not work for a third-party optimimization routine.   ansi-c*/
      if (f0 < -0.5*NEARINFINITY)
      {
         for (i=n-1; i>=0; i--)
            g[i] = GRADMANUAL;
         return;;    /*  Early exit.   ansi-c*/
      }

      dh2i = (dhi=1.0/(dh=grdh))/2.0;
      for (i=0, xp=x; i<n; i++, xp++, g++) {
         tmp = *xp;
         *xp += dh;
/*           //The following statement is bad because dh does not get reset at the beginning of the loop and thus may get changed continually within the loop.   ansi-c*/
/*           //  dh = *xp - tmp;                   // This increases the precision slightly.   ansi-c*/
         fp = fcn(x, t, smodel_ps);
         *xp = tmp - dh;
         fm = fcn(x, t, smodel_ps);

/*           //=== Checking the boundary condition for the minimization problem.   ansi-c*/
         if ((fp > -0.5*NEARINFINITY) && (fm > -0.5*NEARINFINITY))  *g = (fp-fm)*dh2i;
         else if (fp > -0.5*NEARINFINITY)  *g = (fp-f0)*dhi;
         else if (fm > -0.5*NEARINFINITY)  *g = (f0-fm)*dhi;
         else  *g = GRADMANUAL;

         *xp = tmp;                         /*   Put the original value of x[i] back to x[i] so that the content x[i] is still unaltered.   ansi-c*/
      }

   }
   else {
/*        //=== If f0 <= -0.5*NEARINFINITY, we're in a bad region and so we assume it's GRADMANUAL in this bad region. This assumption may or may not work for a third-party optimimization routine.   ansi-c*/
      if (f0 <= -0.5*NEARINFINITY)
      {
         for (i=n-1; i>=0; i--)
            g[i] = GRADMANUAL;
         return;;    /*  Early exit.   ansi-c*/
      }

      for (i=0, xp=x; i<n; i++, xp++, g++) {
         dh = fabs(*xp)<=1 ? STPS : STPS*(*xp);
         tmp = *xp;
         *xp += dh;
         dh = *xp - tmp;                    /*   This increases the precision slightly.   ansi-c*/
         fp = fcn(x, t, smodel_ps);
         *xp = tmp - dh;
         fm = fcn(x, t, smodel_ps);

/*           //=== Checking the boundary condition for the minimization problem.   ansi-c*/
         if ((fp > -0.5*NEARINFINITY) && (fm > -0.5*NEARINFINITY))  *g = (fp-fm)/(2.0*dh);
         else if (fp > -0.5*NEARINFINITY)  *g = (fp-f0)/dh;
         else if (fm > -0.5*NEARINFINITY)  *g = (f0-fm)/dh;
         else  *g = GRADMANUAL;

         *xp = tmp;                         /*   Put the original value of x[i] back to x[i] so that the content x[i] is still unaltered.   ansi-c*/
      }
   }
   g_dv->flag = V_DEF;
}
#undef STPS
#undef GRADMANUAL
/*  //---   ansi-c*/
#if defined (__SWITCHING_VER_200__)
static double logCondPostKernTimet(double *xchange_pd, int t, struct TStateModel_tag *smodel_ps)
{
/*     //Evaluating log conditional posterior kernel at time t -- p(y_t | Y_{t-1}, theta, q).   ansi-c*/
   int fss = smodel_ps->nobs - smodel_ps->fobs + 1;
   double *x1_pd, *x2_pd;


   x1_pd = xchange_pd;
   x2_pd = xchange_pd + NumberFreeParametersTheta(smodel_ps);
/*          //Note that NumberFreeParametersTheta() is DW's function, which points to TZ's function.   ansi-c*/
/*          //In the constant parameter model, this will point to an invalid place,   ansi-c*/
/*          //  but will be taken care of automatically by DW's function ConvertFreeParametersToQ().   ansi-c*/

/*     //======= This is a must step to refresh the value at the new point. =======   ansi-c*/
   ConvertFreeParametersToTheta(smodel_ps, x1_pd);    /*  Waggoner's function, which calls TZ's Convertphi2*().   ansi-c*/
   ConvertFreeParametersToQ(smodel_ps, x2_pd);    /*  Waggoner's function, which automatically takes care of the constant-parameter situition   ansi-c*/
   ThetaChanged(smodel_ps);  /*  DW's function, which will also call my function to set a flag for refreshing everything under these new parameters.   ansi-c*/


   if (1)   /*  Posterior function.   ansi-c*/
      return ( LogConditionalLikelihood_StatesIntegratedOut(t, smodel_ps) + LogPrior(smodel_ps)/((double)fss) );  /*  DW's function.   ansi-c*/
   else //Likelihood (with no prior)
      return ( LogConditionalLikelihood_StatesIntegratedOut(t, smodel_ps) );  /*  DW's function.   ansi-c*/
}
#endif

/*  //------------------------   ansi-c*/
/*  // Computing the Hessian at the log posterior or log likelihood peak, using the outer-product Hessian.   ansi-c*/
/*  //------------------------   ansi-c*/
#if defined (__SWITCHING_VER_200__)
TSdmatrix *ComputeHessianFromOuterProduct(TSdmatrix *Hessian_dm, struct TStateModel_tag *smodel_ps, TSdvector *xhat_dv)
{
/*     //Output:   ansi-c*/
/*     //  Hessian_dm: its inverse equals to Omega (covariance matrix) produced by ComputeCovarianceFromOuterProduct().   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  xhat_dv: Hessian at this point.   ansi-c*/

   int ti;
   double f0;
   int nData = smodel_ps->nobs;
/*     //===   ansi-c*/
   TSdvector *grad_dv;


   grad_dv = CreateVector_lf(xhat_dv->n);
   if (!Hessian_dm)  Hessian_dm = CreateConstantMatrix_lf(xhat_dv->n, xhat_dv->n, 0.0);

/*     //=== Computing the outer-product Hessian.   ansi-c*/
   for (ti=smodel_ps->fobs; ti<=nData; ti++)   /*  Base-1 set-up, thus <=nData, NOT <nData.   ansi-c*/
   {
      f0 = logCondPostKernTimet(xhat_dv->v, ti, smodel_ps);
      gradcd_timet(grad_dv, xhat_dv, ti, smodel_ps, logCondPostKernTimet, 0.0, f0);
      VectorTimesSelf(Hessian_dm, grad_dv, 1.0, 1.0, 'U');
   }


   SUtoGE(Hessian_dm);  /*  Making upper symmetric matarix to a full matrix.   ansi-c*/
   Hessian_dm->flag = M_GE;  /*  Reset this flag, so   ansi-c*/
   ScalarTimesMatrixSquare(Hessian_dm, 0.5, Hessian_dm, 'T', 0.5);   /*  Making it symmetric against some rounding errors.   ansi-c*/
/*                        //This making-symmetric is very IMPORTANT; otherwise, we will get the matrix being singular message   ansi-c*/
/*                        //    and eigenvalues being negative for the SPD matrix, etc.  Then the likelihood becomes either   ansi-c*/
/*                        //    a bad number or a complex number.   ansi-c*/
   Hessian_dm->flag |= M_SU | M_SL;


/*     //===   ansi-c*/
   DestroyVector_lf(grad_dv);

   return (Hessian_dm);
}
/*  //------------------------   ansi-c*/
/*  // Computing the covariance matrix for standard errors at the log posterior or likelihood peak, using the outer-product Hessian.   ansi-c*/
/*  //------------------------   ansi-c*/
TSdmatrix *ComputeCovarianceFromOuterProduct(TSdmatrix *Omega_dm, struct TStateModel_tag *smodel_ps, TSdvector *xhat_dv)
{
/*     //Output:   ansi-c*/
/*     //  Omega_dm: covariance matrix, which equals to the inverse of the Hessian produced by ComputeHessianFromOuterProduct().   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  xhat_dv: Hessian at this point.   ansi-c*/

   int ti;
   double f0;
   int nData = smodel_ps->nobs;
/*     //===   ansi-c*/
   TSdvector *grad_dv;


   grad_dv = CreateVector_lf(xhat_dv->n);
   if (!Omega_dm)  Omega_dm = CreateConstantMatrix_lf(xhat_dv->n, xhat_dv->n, 0.0);

/*     //=== Computing the outer-product Hessian.   ansi-c*/
   for (ti=smodel_ps->fobs; ti<=nData; ti++)   /*  Base-1 set-up, thus <=nData, NOT <nData.   ansi-c*/
   {
      f0 = logCondPostKernTimet(xhat_dv->v, ti, smodel_ps);
      gradcd_timet(grad_dv, xhat_dv, ti, smodel_ps, logCondPostKernTimet, 0.0, f0);
      VectorTimesSelf(Omega_dm, grad_dv, 1.0, 1.0, 'U');
   }
   SUtoGE(Omega_dm);  /*  Making upper symmetric matarix to a full matrix.   ansi-c*/
   ScalarTimesMatrixSquare(Omega_dm, 0.5, Omega_dm, 'T', 0.5);   /*  Making it symmetric against some rounding errors.   ansi-c*/
/*                        //This making-symmetric is very IMPORTANT; otherwise, we will get the matrix being singular message   ansi-c*/
/*                        //    and eigenvalues being negative for the SPD matrix, etc.  Then the likelihood becomes either   ansi-c*/
/*                        //    a bad number or a complex number.   ansi-c*/
   Omega_dm->flag |= M_SU | M_SL;


/*     //--- Converting or inverting the Hessian to covariance.   ansi-c*/
   if (invspd(Omega_dm, Omega_dm, 'U'))
      fn_DisplayError(".../cstz.c/ComputeCovarianceFromOuterProduct(): Hessian must be invertible");


/*     //-- Doubly safe to force it to be symmetric.   ansi-c*/
   SUtoGE(Omega_dm);  /*  Making upper symmetric matarix to a full matrix.   ansi-c*/
   ScalarTimesMatrixSquare(Omega_dm, 0.5, Omega_dm, 'T', 0.5);   /*  Making it symmetric against some rounding errors.   ansi-c*/
/*                        //This making-symmetric is very IMPORTANT; otherwise, we will get the matrix being singular message   ansi-c*/
/*                        //    and eigenvalues being negative for the SPD matrix, etc.  Then the likelihood becomes either   ansi-c*/
/*                        //    a bad number or a complex number.   ansi-c*/
   Omega_dm->flag |= M_SU | M_SL;

/*     //--- Checking if it's symmetric, positive definite.   ansi-c*/


/*     //===   ansi-c*/
   DestroyVector_lf(grad_dv);

   return (Omega_dm);
}



/*  //------------------------   ansi-c*/
/*  // Computing the Hessian at the log posterior or log likelihood peak, using second derivatives.   ansi-c*/
/*  //------------------------   ansi-c*/
TSdmatrix *ComputeHessianFrom2ndDerivative(TSdmatrix *Hessian_dm, struct TStateModel_tag *smodel_ps, TSdvector *xhat_dv)
{
/*     //Output:   ansi-c*/
/*     //  Hessian_dm: its inverse equals to Omega (covariance matrix).   ansi-c*/
/*     //    The flag is set to  M_GE | M_SU | M_SL by hesscd_smodel().   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  xhat_dv: Hessian at this point.   ansi-c*/

   double f0;
   int nData = smodel_ps->nobs;


   if (!Hessian_dm)  Hessian_dm = CreateConstantMatrix_lf(xhat_dv->n, xhat_dv->n, 0.0);

/*     //=== Computing the inner-product Hessian.   ansi-c*/
   f0 = neglogPostKern_hess(xhat_dv->v, smodel_ps);
   hesscd_smodel(Hessian_dm, xhat_dv, smodel_ps, neglogPostKern_hess, 0.0, f0);

   return (Hessian_dm);
}
/*  //---   ansi-c*/
#define STPS 1.0e-4 //6.0554544523933391e-6    /* step size = pow(DBL_EPSILON,1.0/3) */
static void hesscd_smodel(TSdmatrix *H_dm, TSdvector *x_dv, struct TStateModel_tag *smodel_ps, double (*fcn)(double *, struct TStateModel_tag *), double grdh, double f0)
{
/*     //Outputs:   ansi-c*/
/*     //  H_dm: the Hessian n-by-n (no need to be initialized).   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  x_dv: the vector point at which the gradient is evaluated.  No change in the end although will be added or substracted by dh during the function (but in the end the original value will be put back).   ansi-c*/
/*     //  fcn(): the negative (-) log LH or posterior function for which the gradient is evaluated   ansi-c*/
/*     //  grdh: step size.  If 0.0, then dh is set automatically; otherwise, grdh is taken as a step size, often set as 1.0e-004.   ansi-c*/
/*     //  f0: the value of (*fcn)(x).   NOT used in this function except dealing with the boundary (NEARINFINITY) for the   ansi-c*/
/*     //    minimization problem, but to be compatible with a genral function call where, say, gradfw_gen() and cubic   ansi-c*/
/*     //    interpolation of central difference method will use f0.   ansi-c*/

   double dhi, dhj, f1, f2, f3, f4, tmpi, tmpj, *xpi, *xpj;
   int i, j;
/*     //--- Accessible variables.   ansi-c*/
   int n;
   double *H, *x;

   if (!x_dv)  fn_DisplayError(".../cstz.c/hesscd_smodel(): the input x_dv must be allocated memory");
   if (!x_dv->flag)  fn_DisplayError(".../cstz.c/hesscd_smodel(): the input x_dv must be given legal values");
   if (!H_dm)  fn_DisplayError(".../cstz.c/hesscd_smodel(): H_dm must be allocated memory");
   if ( ((n=x_dv->n) != H_dm->nrows) || (n != H_dm->ncols) )  fn_DisplayError(".../cstz.c/hesscd_smodel(): Check the dimension of x_dv and H_dm");

   H = H_dm->M;
   x = x_dv->v;

   for (i=0, xpi=x; i<n; i++, xpi++) {
      dhi = grdh?grdh:(fabs(*xpi)<1?STPS:STPS*(*xpi));
      tmpi = *xpi;
      for (j=i, xpj=x+i; j<n; j++, xpj++)
         if (i==j)
         {
            /* f2 = f3 when i = j */
            if ((f2 = fcn(x, smodel_ps)) > 0.5*NEARINFINITY)  f2 = f0;

            /* this increases precision slightly */
            *xpi += dhi;
            dhi = *xpi - tmpi;

            /* calculate f1 and f4 */
            *xpi = tmpi + 2*dhi;
            if ((f1 = fcn(x, smodel_ps)) > 0.5*NEARINFINITY)  f1 = f0;

            *xpi = tmpi - 2*dhi;
            if ((f4 = fcn(x, smodel_ps)) > 0.5*NEARINFINITY)  f4 = f0;

            /* diagonal element */
            H[i*(n+1)] = (f1-2*f2+f4)/(4*dhi*dhi);

            /* reset to intial value */
            *xpi = tmpi;
         }
         else
         {
            dhj = grdh?grdh:(fabs(*xpj)<1?STPS:STPS*(*xpj));
            tmpj = *xpj;

            /* this increases precision slightly */
            *xpi += dhi;
            dhi = *xpi - tmpi;
            *xpj += dhj;
            dhj = *xpj - tmpj;

            /* calculate f1, f2, f3 and f4 */
            *xpj = tmpj + dhj;
            if ((f1 = fcn(x, smodel_ps)) > 0.5*NEARINFINITY)  f1 = f0;
            *xpi = tmpi - dhi;
            if ((f2 = fcn(x, smodel_ps)) > 0.5*NEARINFINITY)  f2 = f0;
            *xpi = tmpi + dhi;
            *xpj = tmpj - dhj;
            if ((f3 = fcn(x, smodel_ps)) > 0.5*NEARINFINITY)  f3 = f0;
            *xpi = tmpi - dhi;
            if ((f4 = fcn(x, smodel_ps)) > 0.5*NEARINFINITY)  f4 = f0;

            /* symmetric elements */
            H[i+j*n] = H[j+i*n] = (f1-f2-f3+f4)/(4*dhi*dhj);

            /* reset to intial values */
            *xpi = tmpi;
            *xpj = tmpj;
         }
   }

/*     //--- To be safe.   ansi-c*/
   H_dm->flag = M_SU;
   SUtoGE(H_dm);  /*  Making upper symmetric matarix to a full matrix.   ansi-c*/
   H_dm->flag = M_GE;  /*  Reset this flag, so   ansi-c*/

   ScalarTimesMatrixSquare(H_dm, 0.5, H_dm, 'T', 0.5);   /*  Making it symmetric against some rounding errors.   ansi-c*/
/*                        //This making-symmetric is very IMPORTANT; otherwise, we will get the matrix being singular message   ansi-c*/
/*                        //    and eigenvalues being negative for the SPD matrix, etc.  Then the likelihood becomes either   ansi-c*/
/*                        //    a bad number or a complex number.   ansi-c*/
   H_dm->flag |= M_SU | M_SL;
}
#undef STPS
/*  //---   ansi-c*/
static double neglogPostKern_hess(double *xchange_pd, struct TStateModel_tag *smodel_ps)
{
/*     //Evaluating negative log posterior kernel p(y_T | theta, q).   ansi-c*/
   int fss = smodel_ps->nobs - smodel_ps->fobs + 1;
   double *x1_pd, *x2_pd;


   x1_pd = xchange_pd;
   x2_pd = xchange_pd + NumberFreeParametersTheta(smodel_ps);
/*          //Note that NumberFreeParametersTheta() is DW's function, which points to TZ's function.   ansi-c*/
/*          //In the constant parameter model, this will point to an invalid place,   ansi-c*/
/*          //  but will be taken care of automatically by DW's function ConvertFreeParametersToQ().   ansi-c*/

/*     //======= This is a must step to refresh the value at the new point. =======   ansi-c*/
   ConvertFreeParametersToTheta(smodel_ps, x1_pd);    /*  Waggoner's function, which calls TZ's Convertphi2*().   ansi-c*/
   ConvertFreeParametersToQ(smodel_ps, x2_pd);    /*  Waggoner's function, which automatically takes care of the constant-parameter situition   ansi-c*/
   ThetaChanged(smodel_ps);  /*  DW's function, which will also call my function to set a flag for refreshing everything under these new parameters.   ansi-c*/


   if (1)   /*  Posterior function.   ansi-c*/
      return ( -LogLikelihood_StatesIntegratedOut(smodel_ps) - LogPrior(smodel_ps) );  /*  DW's function.   ansi-c*/
   else //Likelihood (with no prior)
      return ( -LogLikelihood_StatesIntegratedOut(smodel_ps) );  /*  DW's function.   ansi-c*/
}
#endif










/*  //????????????????   ansi-c*/
/**
//===
static struct TStateModel_tag *SMODEL_PS = NULL;    //Minimization to find the MLE or posterior peak.
static struct TStateModel_tag *SetModelGlobalForCovariance(struct TStateModel_tag *smodel_ps)
{
   //Returns the old pointer in order to preserve the previous value.
   struct TStateModel_tag *tmp_ps =SMODEL_PS;
   SMODEL_PS = smodel_ps;
   return (tmp_ps);
}
//--- Can be used for conjugate gradient minimization as well.
static double ObjFuncForSmodel(double *x0_p, int d_x0)
{
   TSdvector x0_sdv;
   x0_sdv.v = x0_p;
   x0_sdv.n = d_x0;
   x0_sdv.flag = V_DEF;

   return ( -opt_logOverallPosteriorKernal(SMODEL_PS, &x0_sdv) );
}
//---
static double opt_logOverallPosteriorKernal(struct TStateModel_tag *smodel_ps, TSdvector *xchange_dv)
{
   double *x1_pd, *x2_pd;


   x1_pd = xchange_dv->v;
   x2_pd = xchange_dv->v + NumberFreeParametersTheta(smodel_ps);
        //Note that NumberFreeParametersTheta() is DW's function, which points to TZ's function.
        //In the constant parameter model, this will point to invalid,
        //  but will be taken care of automatically by DW's function ConvertFreeParametersToQ().

   //======= This is a must step to refresh the value at the new point. =======
   ConvertFreeParametersToTheta(smodel_ps, x1_pd);   //Waggoner's function, which calls TZ's Convertphi2*().
   ConvertFreeParametersToQ(smodel_ps, x2_pd);   //Waggoner's function, which automatically takes care of the constant-parameter situition
   ThetaChanged(smodel_ps); //DW's function, which will also call my function to set a flag for refreshing everything under these new parameters.
   if (1)  //Posterior function.
      return ( LogPosterior_StatesIntegratedOut(smodel_ps) ); //DW's function.
   else //Likelihood (with no prior)
      return ( LogLikelihood_StatesIntegratedOut(smodel_ps) ); //DW's function.
}
/**/








int next_permutation(int *first, int *last)
{
/*     // Given the permulation, say, [3 2 1 0], the ouput is the next permulation [0 1 2 3], and so on.   ansi-c*/
/*     // Note that last is simply a pointer.  Because it is not allocated to a memory, it cannot be accessed.   ansi-c*/
/*     //   So last is used for (1) gauging the dimension size of the array first;   ansi-c*/
/*     //                       (2) being accssed but with --last (which points to a valid memory place), NOT last.   ansi-c*/
/*     //   ansi-c*/
/*     // first: n-by-1 vector of integers filled with 0, 1, 2, ..., n.   ansi-c*/
/*     // last:  simply a pointer to the address after the last element of first.  Note that no memory is allocated.   ansi-c*/

   int *i = last, *ii, *j, tmp;
   if (first == last || first == --i)
      return 0;

   for(; ; ) {
      ii = i;
      if (*--i < *ii) {
         j = last;
         while (!(*i < *--j));
         tmp = *i; *i = *j; *j = tmp;
         for (; ii != last && ii != --last; ++ii) {
            tmp = *ii; *ii = *last; *last = tmp;
         }
         return 1;
      }
      if (i == first) {
         for (; first != last && first != --last; ++first) {
            tmp = *first; *first = *last; *last = tmp;
         }
         return 0;
      }
   }
}



/**
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void permute_matrix(double *a, int n, int *indx) {
   double *b;
   int nn=n*n;
   register int i;
   b = swzCalloc(nn,sizeof(double));
   memcpy(b, a, nn*sizeof(double));
   for (i=0; i<nn; i++, a++)
      *a = b[indx[i%n]+indx[i/n]*n];
}

int main() {
   double a[9]={1,2,3,4,5,6,7,8,9};
   int indx[3]={1,2,0};
   permute_matrix(a,3,indx);
   return 0;
}
/**/


int fn_cumsum_int(int *x_v, const int d_x_v) {
/*     //Outputs:   ansi-c*/
/*     //  x_v: an int vector of cumulative sums over an input int vector.   ansi-c*/
/*     //  return: the sum of an input int vector.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  x_v: a vector of ints.   ansi-c*/
/*     //  d_x_v: dimension of x_v.   ansi-c*/
/*     //   ansi-c*/
/*     // Compute cumulative sums of a vector of ints.   ansi-c*/
   int _i;

   if (x_v==NULL) fn_DisplayError(".../cstz/fn_cumsum_lf:  x_v must be allocated with memory");

   for (_i=1; _i<d_x_v; _i++) {
      x_v[_i] = x_v[_i-1] + x_v[_i];
   }

   return (x_v[d_x_v-1]);
}


double fn_cumsum_lf(double *x_v, const int d_x_v) {
/*     //Outputs:   ansi-c*/
/*     //  x_v: a double vector of cumulative sums over an input double vector.   ansi-c*/
/*     //  return: the sum of an input double vector.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  x_v: a vector of doubles.   ansi-c*/
/*     //  d_x_v: dimension of x_v.   ansi-c*/
/*     //   ansi-c*/
/*     // Compute cumulative sums of a vector of doubles.   ansi-c*/
   int _i;

   if (!x_v) fn_DisplayError(".../cstz/fn_cumsum_lf:  x_v must be allocated with memory");

   for (_i=1; _i<d_x_v; _i++) {
      x_v[_i] = x_v[_i-1] + x_v[_i];
   }

   return (x_v[d_x_v-1]);
}


double fn_mean(const double *a_v, const int _n) {
   int _i;
   double x=0.0;

   for (_i=0; _i<_n; _i++)  x += a_v[_i];
   x /= (double)_n;

   return x;
}

/*  //<<---------------   ansi-c*/
static double *tz_BaseForComp;           /*   This base variable is to be sorted and thus made global for this source file.   ansi-c*/
void fn_SetBaseArrayForComp(TSdvector *x_dv)
{
   if ( !x_dv->flag )   fn_DisplayError(".../cstz.c/ftd_SetBaseArrayForComp(): input vector used for comparison must be given legal values");
   else  tz_BaseForComp = x_dv->v;
}
int fn_compare(const void *i1, const void *i2)
{
/*     // Ascending order according to tz_BaseForComp.   ansi-c*/
   return ( (tz_BaseForComp[*((int*)i1)]<tz_BaseForComp[*((int*)i2)]) ? -1 : (tz_BaseForComp[*((int*)i1)]>tz_BaseForComp[*((int*)i2)]) ? 1 : 0 );
}
int fn_compare2(const void *i1, const void *i2)
{
/*     // Descending order according to tz_BaseForComp.   ansi-c*/
   return ( (tz_BaseForComp[*((int*)i1)]<tz_BaseForComp[*((int*)i2)]) ? 1 : (tz_BaseForComp[*((int*)i1)]>tz_BaseForComp[*((int*)i2)]) ? -1 : 0);
}
/*  //======= Quick sort. =======   ansi-c*/
static int ftd_CompareDouble(const void *a, const void *b)
{
/*     // Ascending order for the series that contains a and b.   ansi-c*/
   return (*(double *)a < *(double *)b ? -1 : *(double *)a > *(double *)b ? 1 : 0);
}
static int ftd_CompareDouble2(const void *a, const void *b)
{
/*     // Dscending order for the series that contains a and b.   ansi-c*/
   return (*(double *)a < *(double *)b ? 1 : *(double *)a > *(double *)b ? -1 : 0);
}
/*  //---   ansi-c*/
void tz_sort(TSdvector *x_dv, char ad)
{
/*     //x_dv will be replaced by the sorted value.   ansi-c*/
/*     //Sort x_dv according to the descending or ascending order indicated by ad.   ansi-c*/
/*     //ad == "A' or 'a': acending order.   ansi-c*/
/*     //ad == 'D' or 'd': descending order.   ansi-c*/
   if (!x_dv || !x_dv->flag)  fn_DisplayError("cstz.c/tz_sort(): input vector x_dv must be (1) created and (2) assigned values");

   qsort( (void *)x_dv->v, (size_t)x_dv->n, sizeof(double), ((ad=='A') || (ad=='a')) ? ftd_CompareDouble : ftd_CompareDouble2);
}
void tz_sortindex_lf(TSivector *x_iv, TSdvector *base_dv, char ad)
{
/*     //???????NOT fully tested yet.   ansi-c*/
/*     //x_iv will be replaced by the sorted integer vector.   ansi-c*/
/*     //base_dv will not be affected.   ansi-c*/
/*     //Sort x_iv according to the descending or ascending order of base_dv.   ansi-c*/
/*     //ad == "A' or 'a': acending order.   ansi-c*/
/*     //ad == 'D' or 'd': descending order.   ansi-c*/
   if (!x_iv || !base_dv || !x_iv->flag || !base_dv->flag)  fn_DisplayError("cstz.c/tz_sortindex(): input vectors x_iv and base_dv must be (1) created and (2) assigned values");
   if (x_iv->n != base_dv->n)  fn_DisplayError("cstz.c/tz_sortindex(): lengths of the two input vectors must be the same");

   fn_SetBaseArrayForComp(base_dv);
   qsort( (void *)x_iv->v, (size_t)x_iv->n, sizeof(int), ((ad=='A') || (ad=='a')) ? fn_compare : fn_compare2);
}
void tz_sortindex(TSivector *x_iv, TSvoidvector *base_voidv, char ad)
{
/*     //???????NOT fully tested yet.   ansi-c*/
/*     //Allowing x_iv = base_voidv or sets base_voidv=NULL   ansi-c*/
/*     //Sort x_iv according to the descending or ascending order of base_voidv.   ansi-c*/
/*     //ad == "A' or 'a': acending order.   ansi-c*/
/*     //ad == 'D' or 'd': descending order.   ansi-c*/
   if (!x_iv || !base_voidv || !x_iv->flag || !base_voidv->flag)  fn_DisplayError("cstz.c/tz_sort_int(): input vectors x_iv and base_voidv must be (1) created and (2) assigned values");
   if (x_iv->n != base_voidv->n)  fn_DisplayError("cstz.c/tz_sort_int(): lengths of the two input vectors must be the same");

   fn_SetBaseArrayForComp((TSdvector *)base_voidv);
   qsort( (void *)x_iv->v, (size_t)x_iv->n, sizeof(int), ((ad=='A') || (ad=='a')) ? fn_compare : fn_compare2);
}
/*  //---   ansi-c*/
void tz_sort_matrix(TSdmatrix *X_dm, char ad, char rc)
{
/*     //Fast method: rc = 'C' (sort each column).   ansi-c*/
/*     //Output: X_dm will be replaced by the sorted value.   ansi-c*/
/*     //  Sort X_dm (1) by columns or rows indicated by rc and (2) according to the descending or ascending order indicated by ad.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  ad == 'A' or 'a': acending order.   ansi-c*/
/*     //  ad == 'D' or 'd': descending order.   ansi-c*/
/*     //  rc == 'C' or 'c': sort each column.   ansi-c*/
/*     //  rc == 'R' or 'r': sort each row.   ansi-c*/
   int nrows, ncols, _j, begloc;
   TSdvector x_sdv;
   double *X;
/*     //===   ansi-c*/
   TSdmatrix *Xtran_dm = NULL;

   if (!X_dm || !(X_dm->flag & M_GE))  fn_DisplayError("cstz.c/tz_sort_matrix(): input matrix X_dm must be (1) created and (2) assigned values and (3) regular (M_GE)");
   x_sdv.flag = V_DEF;

   if (rc=='C' || rc=='c')
   {
      X = X_dm->M;
      nrows = X_dm->nrows;
      ncols = X_dm->ncols;
   }
   else
   {
      Xtran_dm = tz_TransposeRegular((TSdmatrix *)NULL, X_dm);
      X = Xtran_dm->M;
      nrows = Xtran_dm->nrows;
      ncols = Xtran_dm->ncols;
   }
   x_sdv.n = nrows;
   for (begloc=nrows*(ncols-1), _j=ncols-1; _j>=0; begloc-=nrows, _j--)
   {
      x_sdv.v = X + begloc;
      tz_sort(&x_sdv, ad);
   }

   if (rc=='R' || rc=='r')
   {
      tz_TransposeRegular(X_dm, Xtran_dm);
/*        //===   ansi-c*/
      DestroyMatrix_lf(Xtran_dm);
   }
}
/*  //---   ansi-c*/
TSdvector *tz_prctile_matrix(TSdvector *z_dv, const double prc, TSdmatrix *Z_dm, const char rc)
{
/*     //Fast method: rc = 'C' (sort each column).   ansi-c*/
/*     //Output:  %prc percentile (i.e., containing 0% to %prc).   ansi-c*/
/*     //  z_dv: an n-by-1 vector if rc=='C' or an m-by-1 vector if rc=='R'.   ansi-c*/
/*     //  If z_dv==NULL, it will be created and has to be destroyed outside this function.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  prc: percent (must be between 0.0 and 1.0 inclusive).   ansi-c*/
/*     //  X_dm: an m-by-n general matrix.   ansi-c*/
/*     //  rc == 'C' or 'c': sort each column.   ansi-c*/
/*     //  rc == 'R' or 'r': sort each row.   ansi-c*/
   int nrows, ncols, _j, begloc;
   TSdvector x_sdv;
   double *X;
/*     //===   ansi-c*/
   TSdmatrix *X_dm = NULL;
   TSdmatrix *Xtran_dm = NULL;

   if (!Z_dm || !Z_dm->flag)  fn_DisplayError("cstz.c/tz_prctile_matrix(): input matrix Z_dm must be (1) created and (2) assigned values");
   if (prc<0.0 || prc>1.0)  fn_DisplayError("cstz.c/tz_prctile_matrix(): percentile mark prc must be between 0.0 and 1.0 inclusive");
   x_sdv.flag = V_DEF;

   nrows = Z_dm->nrows;
   ncols = Z_dm->ncols;
   if (!z_dv)
   {
      if (rc=='C' || rc=='c')  z_dv = CreateVector_lf(ncols);
      else  z_dv = CreateVector_lf(nrows);
   }
   else
   {
      if ((rc=='C' || rc=='c'))
      {
         if (ncols != z_dv->n)  fn_DisplayError("cstz.c/tz_prctile_matrix(): z_dv->n must be the same as ncols of X_dm when sorting each column");
      }
      else
      {
         if (nrows != z_dv->n)  fn_DisplayError("cstz.c/tz_prctile_matrix(): z_dv->n must be the same as nrows of X_dm when sorting each row");
      }
   }
   X_dm = CreateMatrix_lf(nrows, ncols);
   CopyMatrix0(X_dm, Z_dm);

   if (rc=='C' || rc=='c')
   {
      X = X_dm->M;
      nrows = X_dm->nrows;
      ncols = X_dm->ncols;
   }
   else
   {
      Xtran_dm = tz_TransposeRegular((TSdmatrix *)NULL, X_dm);
      X = Xtran_dm->M;
      nrows = Xtran_dm->nrows;
      ncols = Xtran_dm->ncols;
   }
   x_sdv.n = nrows;
   for (begloc=nrows*(ncols-1), _j=ncols-1; _j>=0; begloc-=nrows, _j--)
   {
      x_sdv.v = X + begloc;
      tz_sort(&x_sdv, 'A');
      z_dv->v[_j] = x_sdv.v[(int)floor(prc*(double)nrows)];
   }
   z_dv->flag = V_DEF;
   if (rc=='R' || rc=='r')  DestroyMatrix_lf(Xtran_dm);

/*     //===   ansi-c*/
   DestroyMatrix_lf(X_dm);

   return (z_dv);
}
/*  //---   ansi-c*/
TSdvector *tz_mean_matrix(TSdvector *z_dv, TSdmatrix *Z_dm, const char rc)
{
/*     //Fast method: rc = 'C' (mean for each column).   ansi-c*/
/*     //Output:  %prc percentile (i.e., containing 0% to %prc).   ansi-c*/
/*     //  z_dv: an n-by-1 vector if rc=='C' or an m-by-1 vector if rc=='R'.   ansi-c*/
/*     //  If z_dv==NULL, it will be created and has to be destroyed outside this function.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  X_dm: an m-by-n general matrix.   ansi-c*/
/*     //  rc == 'C' or 'c': mean for each column.   ansi-c*/
/*     //  rc == 'R' or 'r': mean for each row.   ansi-c*/
   int nrows, ncols, _j, begloc;
   TSdvector x_sdv;
   double *X;
/*     //===   ansi-c*/
   TSdmatrix *X_dm = NULL;
   TSdmatrix *Xtran_dm = NULL;

   if (!Z_dm || !Z_dm->flag)  fn_DisplayError("cstz.c/tz_mean_matrix(): input matrix Z_dm must be (1) created and (2) assigned values");
   x_sdv.flag = V_DEF;

   nrows = Z_dm->nrows;
   ncols = Z_dm->ncols;
   if (!z_dv)
   {
      if (rc=='C' || rc=='c')  z_dv = CreateVector_lf(ncols);
      else  z_dv = CreateVector_lf(nrows);
   }
   else
   {
      if ((rc=='C' || rc=='c'))
      {
         if (ncols != z_dv->n)  fn_DisplayError("cstz.c/tz_mean_matrix(): z_dv->n must be the same as ncols of X_dm when computing mean for each column");
      }
      else
      {
         if (nrows != z_dv->n)  fn_DisplayError("cstz.c/tz_mean_matrix(): z_dv->n must be the same as nrows of X_dm when computing mean for  each row");
      }
   }
   X_dm = CreateMatrix_lf(nrows, ncols);
   CopyMatrix0(X_dm, Z_dm);

   if (rc=='C' || rc=='c')
   {
      X = X_dm->M;
      nrows = X_dm->nrows;
      ncols = X_dm->ncols;
   }
   else
   {
      Xtran_dm = tz_TransposeRegular((TSdmatrix *)NULL, X_dm);
      X = Xtran_dm->M;
      nrows = Xtran_dm->nrows;
      ncols = Xtran_dm->ncols;
   }
   x_sdv.n = nrows;
   for (begloc=nrows*(ncols-1), _j=ncols-1; _j>=0; begloc-=nrows, _j--)
   {
      x_sdv.v = X + begloc;
      z_dv->v[_j] = fn_mean(x_sdv.v, x_sdv.n);
   }
   z_dv->flag = V_DEF;
   if (rc=='R' || rc=='r')  DestroyMatrix_lf(Xtran_dm);

/*     //===   ansi-c*/
   DestroyMatrix_lf(X_dm);

   return (z_dv);
}
/*  //--------------->>   ansi-c*/



/*  //<<---------------   ansi-c*/
/*  // WZ normalization on VARs.   ansi-c*/
/*  //--------------->>   ansi-c*/
void fn_wznormalization(TSdvector *wznmlz_dv, TSdmatrix *A0draw_dm, TSdmatrix *A0peak_dm)
{
/*     //Outputs:   ansi-c*/
/*     //  wznmlz_dv (n-by-1):  If negative, the sign of the equation must switch; if positive: no action needs be taken.   ansi-c*/
/*     //    If NULL as an input, remains NULL.   ansi-c*/
/*     //  A0draw_dm (n-by-n):  replaced by wz-normalized draw.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  wznmlz_dv (n-by-1):  if NULL, no output for wznmlz_dv; otherwise, a memory allocated vector.   ansi-c*/
/*     //  A0draw_dm (n-by-n):  a draw of A0.   ansi-c*/
/*     //  A0peak_dm (n-by-n):  reference point to which normalized A0draw_dm is closest.   ansi-c*/
   int _j, _n,
       errflag = -2;
   double *v;
   TSdmatrix *X_dm = NULL;
   TSdvector *diagX_dv = NULL;

   if ( !A0peak_dm )  fn_DisplayError(".../cstz.c/fn_wznormalization():  input matrix for ML estimates must be created (memory allocated) and have legal values");
/*          //This is a minimum check to prevent crash without error messages.  More robust checks are done in BdivA_rgens().   ansi-c*/

   _n = A0peak_dm->nrows;
   X_dm = CreateMatrix_lf(_n, _n);

   if ( errflag=BdivA_rgens(X_dm, A0peak_dm, '\\', A0draw_dm) ) {
      printf(".../cstz.c/fn_wznormalization(): errors when calling BdivA_rgens() with error flag %d", errflag);
      swzExit(EXIT_FAILURE);
   }

   if (wznmlz_dv) {
      diagdv(wznmlz_dv, X_dm);
      v = wznmlz_dv->v;
   }
   else {
      diagX_dv = CreateVector_lf(_n);
      diagdv(diagX_dv, X_dm);
      v = diagX_dv->v;
   }


   for (_j=_n-1; _j>=0; _j--)
      if (v[_j]<0)  ScalarTimesColofMatrix((TSdvector *)NULL, -1.0, A0draw_dm, _j);

/*     //=== Destroys memory allocated for this function only.   ansi-c*/
   DestroyMatrix_lf(X_dm);
   DestroyVector_lf(diagX_dv);
}




/*  //---------------<<   ansi-c*/
/*  // Handling under or over flows with log values.   ansi-c*/
/*  //--------------->>   ansi-c*/
struct TSveclogsum_tag *CreateVeclogsum(int n)
{
   struct TSveclogsum_tag *veclogsum_ps = tzMalloc(1, struct TSveclogsum_tag);

/*     //=== Memory allocation and initialization.   ansi-c*/
   veclogsum_ps->n = n;   /*  Number of sums or the dimension of logofsum.   ansi-c*/
   veclogsum_ps->N_iv = CreateConstantVector_int(n, 0);      /*  Cumulative.  (N_1, ..., N_n).   ansi-c*/
   veclogsum_ps->logsum_dv = CreateConstantVector_lf(n, -MACHINEINFINITY);    /*  Cumulative.  (logofsum_1, ..., logofsum_n).   ansi-c*/
   veclogsum_ps->logmax_dv = CreateConstantVector_lf(n, -MACHINEINFINITY);    /*  (logmax_1, ..., logmax_n).   ansi-c*/

   return (veclogsum_ps);
}
/*  //---   ansi-c*/
struct TSveclogsum_tag *DestroyVeclogsum(struct TSveclogsum_tag *veclogsum_ps)
{

   if (veclogsum_ps) {
      DestroyVector_int(veclogsum_ps->N_iv);
      DestroyVector_lf(veclogsum_ps->logsum_dv);
      DestroyVector_lf(veclogsum_ps->logmax_dv);

/*        //===   ansi-c*/
      swzFree(veclogsum_ps);
      return ((struct TSveclogsum_tag *)NULL);
   }
   else  return (veclogsum_ps);
}
/*  //===   ansi-c*/
/*  //------------------   ansi-c*/
/*  //Updating the sum (not divided by n) for the mean and the second moment.   ansi-c*/
/*  //------------------   ansi-c*/
void UpdateSumFor1st2ndMoments(TSdvector *x1stsum_dv, TSdmatrix *X2ndsum_dm, const TSdvector *xdraw_dv)
{
   static int ini_indicator = 0;

   if (!ini_indicator) {
/*        //Pass this loop once and no more.   ansi-c*/
      CopyVector0(x1stsum_dv, xdraw_dv);
      VectorTimesSelf(X2ndsum_dm, xdraw_dv, 1.0, 0.0, 'U');
      ini_indicator = 1;
   }
   else {
      VectorPlusVectorUpdate(x1stsum_dv, xdraw_dv);
      VectorTimesSelf(X2ndsum_dm, xdraw_dv, 1.0, 1.0, 'U');
   }
}
/*  //---   ansi-c*/
int tz_update_logofsum(double *Y_N_dp, double *y_Nmax_dp, double ynew, int N)
{
/*     //Recursive algorithm to update Y_N (=log(sum of x_i)) for i=1, ..., N with the new value ynew = log(x_{N+1}).   ansi-c*/
/*     //Returns (1) the updated value Y_{N+1} = log(sum of x_i)) for i=1, ..., N+1;   ansi-c*/
/*     //        (2) the updated value y_(N+1)max_dp;   ansi-c*/
/*     //        (3) the integer N+1.   ansi-c*/
/*     //See TVBVAR Notes p.81a.   ansi-c*/

   if (*y_Nmax_dp>=ynew)  *Y_N_dp = log( exp(*Y_N_dp - *y_Nmax_dp) + exp(ynew - *y_Nmax_dp) ) + *y_Nmax_dp;
   else {
      *y_Nmax_dp = ynew;
      *Y_N_dp = log( exp(*Y_N_dp - ynew) + 1.0 ) + ynew;
   }

   return (N+1);
}
int fn_update_logofsum(int N, double ynew, double *Y_N_dp, double *y_Nmax_dp)
{
/*     //Recursive algorithm to update Y_N (=log(sum of x_i)) for i=1, ..., N with the new value ynew = log(x_{N+1}).   ansi-c*/
/*     //Returns (1) the updated value Y_{N+1} = log(sum of x_i)) for i=1, ..., N+1;   ansi-c*/
/*     //        (2) the updated value y_(N+1)max_dp;   ansi-c*/
/*     //        (3) the integer N+1.   ansi-c*/
/*     //See TVBVAR Notes p.81a.   ansi-c*/
/*     //If N=0, then ynew = -infty (no value yet) and thus no value is added to *Y_N_dp.   ansi-c*/

/*  //   if (N>0)   ansi-c*/
/*  //   {   ansi-c*/
      if (*y_Nmax_dp>=ynew)  *Y_N_dp = log( exp(*Y_N_dp - *y_Nmax_dp) + exp(ynew - *y_Nmax_dp) ) + *y_Nmax_dp;
      else {
         *y_Nmax_dp = ynew;
         *Y_N_dp = log( exp(*Y_N_dp - ynew) + 1.0 ) + ynew;
      }
/*  //   }   ansi-c*/

   return (N+1);
}
double fn_replace_logofsumsbt(double *yold, double _a, double ynew, double _b)
{
/*     //Outputs:   ansi-c*/
/*     //  *yold is replaced by log abs(a*xold + b*xnew).   ansi-c*/
/*     //  1.0 or -1.0: sign of a*xold + b*xnew.   ansi-c*/
/*     //   ansi-c*/
/*     //Given yold=log(xold) and ynew=log(xnew), it updates and returns yold = log abs(a*xold + b*xnew).   ansi-c*/
/*     //sbt: subtraction or subtract.   ansi-c*/
/*     //See TVBVAR Notes p.81a.   ansi-c*/
   double tmpd;
/*     // *yold = (*yold > ynew) ? (log( _a + _b*exp(ynew - *yold)) + *yold) : (log( _a*exp(*yold - ynew) + _b) + ynew);   ansi-c*/

   if (*yold > ynew) {
      if ((tmpd=_a + _b*exp(ynew - *yold) ) < 0.0) {
/*           // printf("WARNING! .../cstz.c/fn_replace_logofsumsbt(): Expression inside log is negative and the function returns the negative sign!\n");   ansi-c*/
         *yold += log(fabs(tmpd));
         return (-1.0);
      }
      else {
         *yold += log(tmpd);
         return (1.0);
      }
   }
   else {
      if ((tmpd=_a*exp(*yold - ynew) + _b) < 0.0 ) {
/*           // printf("WARNING! .../cstz.c/fn_replace_logofsumsbt(): Expression inside log is negative and the function returns the negative sign!\n");   ansi-c*/
         *yold = log(fabs(tmpd)) + ynew;
         return (-1.0);
      }
      else {
         *yold = log(tmpd) + ynew;
         return (1.0);
      }
   }
}


/*  //<<---------------   ansi-c*/
/*  // Evaluating the inverse of the chi-square cumulative distribution function.   ansi-c*/
/*  //--------------->>   ansi-c*/
double fn_chi2inv(double p, double df)
{
#if defined( IMSL_STATISTICSTOOLBOX )
/*     //Returns x where p = int_{0}^{x} chi2pdf(t, df) dt   ansi-c*/
   if (df<=0.0)  fn_DisplayError("cstz.c/fn_chi2inv(): degrees of freedom df must be greater than 0.0");

   if (p<=0.0)  return (0.0);
   else if (p>=1.0)  return (MACHINEINFINITY);
   else  return (imsls_d_chi_squared_inverse_cdf(p, df));
#elif defined( USE_GSL_LIBRARY )
   if (df<=0.0)  fn_DisplayError("cstz.c/fn_chi2inv(): degrees of freedom df must be greater than 0.0");

   if (p<=0.0)  return (0.0);
   else if (p>=1.0)  return (MACHINEINFINITY);
   else
   return gsl_cdf_chisq_Pinv(p,df);
#else
  ***No default routine yet;
#endif
}


/*  //<<---------------   ansi-c*/
/*  // Evaluating the standard normal cumulative distribution function.   ansi-c*/
/*  //--------------->>   ansi-c*/
double fn_normalcdf(double x)
{
#if defined( IMSL_STATISTICSTOOLBOX )
   return (imsls_d_normal_cdf(x));
#elif defined( USE_GSL_LIBRARY )
   return gsl_cdf_ugaussian_P(x);
#else
  ***No default routine yet;
#endif
}


/*  //<<---------------   ansi-c*/
/*  // Evaluating the inverse of the standard normal cumulative distribution function.   ansi-c*/
/*  //--------------->>   ansi-c*/
double fn_normalinv(double p)
{
#if defined( IMSL_STATISTICSTOOLBOX )
   return (imsls_d_normal_inverse_cdf(p));
#elif defined( USE_GSL_LIBRARY )
   return gsl_cdf_ugaussian_Pinv(p);
#else
  ***No default routine yet;
#endif
}


/*  //<<---------------   ansi-c*/
/*  // Evaluating the inverse of the beta cumulative distribution function.   ansi-c*/
/*  //--------------->>   ansi-c*/
double fn_betainv(double p, double _alpha, double _beta)
{
#if defined( IMSL_STATISTICSTOOLBOX )
/*     //p = int_{0}^{\infty} betapdf(t, _alpha, _beta) dt where betapdf(t,_alpha,_beta) \propt t^{_alpha-1}*(1-t)^(_beta-1}.   ansi-c*/
   return (imsls_d_beta_inverse_cdf(p, _alpha, _beta));
#elif defined( USE_GSL_LIBRARY)
   return gsl_cdf_beta_Pinv(p,_alpha,_beta);
#else
  ***No default routine yet;
#endif
}


/*  //<<---------------   ansi-c*/
/*  // Computes log gamma (x) where gamma(n+1) = n! and gamma(x) = int_0^{\infty} e^{-t} t^{x-1} dt.   ansi-c*/
/*  //--------------->>   ansi-c*/
double fn_gammalog(double x)
{
#if defined( IMSL_STATISTICSTOOLBOX )
   return (imsl_d_log_gamma(x));
#elif defined( USE_GSL_LIBRARY )
  return gsl_sf_lngamma(x);
#else
  ***No default routine yet;
#endif
}


/*  //<<---------------   ansi-c*/
/*  // Computes log beta(x, y) where beta(x, y) = gamma(x)*gamm(y)/gamma(x+y).   ansi-c*/
/*  //--------------->>   ansi-c*/
double fn_betalog(double x, double y)
{
#if defined( IMSL_STATISTICSTOOLBOX )
   return (imsl_d_log_beta(x, y));
#elif defined( USE_GSL_LIBRARY )
   return gsl_sf_lnbeta(x,y);
#else
  ***No default routine yet;
#endif
}



/*  //<<---------------   ansi-c*/
/*  // Computes log gamma (x) where gamma(n+1) = n! and gamma(x) = int_0^{\infty} e^{-t} t^{x-1} dt.   ansi-c*/
/*  //--------------->>   ansi-c*/
double gammalog(double x)
{
#if defined( IMSL_STATISTICSTOOLBOX )
   return (imsl_d_log_gamma(x));
#elif defined( USE_GSL_LIBRARY )
  return gsl_sf_lngamma(x);
#else
  ***No default routine yet;
#endif
}


/*  //-----------------------------------------------------------------------------------   ansi-c*/
/*  //------------------------------ Normal distribution ------------------------------//   ansi-c*/
/*  //--- p(x) = (1.0/sqrt(2*pi)*sigma) exp( -(1.0/(2.0*sigma^2.0)) (x-mu)^2.0 )   ansi-c*/
/*  //---         for sigma>0.   ansi-c*/
/*  //-----------------------------------------------------------------------------------   ansi-c*/
#define LOGSQRTOF2PI  9.189385332046727e-001
double tz_lognormalpdf(double _x, double _m, double _s)
{
   double xmm = _x-_m;
   if (_s <= 0.0)  return (-NEARINFINITY);
/*        //fn_DisplayError("cstz.c/tz_lognormalpdf(): standard deviation must be positive");   ansi-c*/

   return ( -LOGSQRTOF2PI - log(_s) - (1.0/(2.0*square(_s))) * square(xmm) );
}
#undef LOGSQRTOF2PI

/*  //-----------------------------------------------------------------------------------   ansi-c*/
/*  //----------------------------- Beta density function -----------------------------//   ansi-c*/
/*  //--- p(x) = ( Gamma(a+b)/(Gamma(a)*Gamma(b)) ) x^(a-1) (1-x)^(b-1) for a>0 and b>0.   ansi-c*/
/*  //--- E(x) = a/(a+b);  var(x) = a*b/( (a+b)^2*(a+b+1) );   ansi-c*/
/*  //--- The density is finite if a,b>=1.   ansi-c*/
/*  //--- Noninformative density: (1) a=b=1; (2) a=b=0.5; or (3) a=b=0.   ansi-c*/
/*  //-----------------------------------------------------------------------------------   ansi-c*/
double tz_logbetapdf(double _x, double _a, double _b)
{
   if ((_x < 0.0) || (_x > 1.0) || (_a <=0.0) || (_b <= 0.0))   return (-NEARINFINITY);
   if ((_x <= 0.0) && (_a != 1.0))  return (-NEARINFINITY);
/*        //Note that it should be +infinity for a < 1.0.  We return -infinity anyway for the purpose of giving zero LH.   ansi-c*/
   if ((_x >= 1.0) && (_b != 1.0))  return (-NEARINFINITY);
/*        //Note that it should be +infinity for b < 1.0.  We return -infinity anyway for the purpose of giving zero LH.   ansi-c*/
/*        //fn_DisplayError("cstz.c/tz_logbetapdf(): x must be (0,1) and a, b must be positive");   ansi-c*/

   if ((_x == 0.0 && _a == 1.0) || (_x == 1.0 && _b == 1.0))  return (-fn_betalog(_a, _b));
   else  return ( -fn_betalog(_a, _b) + (_a-1.0)*log(_x) + (_b-1.0)*log(1.0-_x) );
}
/*  //-----------------------------------------------------------------------------------   ansi-c*/
/*  //---------------------------- Gamma distribution ----------------------------------//   ansi-c*/
/*  //--- p(x) = ( b^a/Gamma(a) ) x^(a-1) exp(-bx) for a>0 and b>0.   ansi-c*/
/*  //---    where a is shape and b is inverse scale (rate) parameter.   ansi-c*/
/*  //--- E(x) = a/b;  var(x) = a/b^2;   ansi-c*/
/*  //--- Noninformative distribution: a,b -> 0.   ansi-c*/
/*  //--- The density function is finite if a >= 1.   ansi-c*/
/*  //-----------------------------------------------------------------------------------   ansi-c*/
double tz_loggammapdf(double _x, double _a, double _b)
{
   if (_x < 0.0 || _a <= 0.0 || _b <= 0.0)  return (-NEARINFINITY);
   if (_x <= 0.0 && _a != 1.0)  return (-NEARINFINITY);
/*        //Note that it should be +infinity for a < 1.0.  We return -infinity anyway for the purpose of giving zero LH.   ansi-c*/
/*        //fn_DisplayError("cstz.c/tz_loggammapdf(): x, a, and b must be positive");   ansi-c*/

   if (_x == 0.0 && _a == 1.0)  return ( _a*log(_b) - fn_gammalog(_a) );
   else  return ( _a*log(_b) - fn_gammalog(_a) + (_a-1.0)*log(_x) - _b*_x );
}
/*  //-----------------------------------------------------------------------------------   ansi-c*/
/*  //------------------------ Inverse-Gamma distribution ------------------------------//   ansi-c*/
/*  //--- p(x) = ( b^a/Gamma(a) ) x^(-a-1) exp(-b/x) for a>0 and b>0.   ansi-c*/
/*  //---    where a is shape and b is scale parameter.   ansi-c*/
/*  //--- E(x) = b/(a-1) for a>1;  var(x) = b^2/( (a-1)^2*(a-2) ) for a>2;   ansi-c*/
/*  //--- Noninformative distribution: a,b -> 0.   ansi-c*/
/*  //--- How to draw: (1) draw z from Gamma(a,b); (2) let x=1/z.   ansi-c*/
/*  //-----------------------------------------------------------------------------------   ansi-c*/
double tz_loginversegammapdf(double _x, double _a, double _b)
{
/*     //This denisity is always finite.   ansi-c*/
/*     //If a < 1.0, 1st moment does not exist,   ansi-c*/
/*     //   a < 2.0, 2nd moment does not exist,   ansi-c*/
/*     //   a < 3.0, 3rd moment does not exist,   ansi-c*/
/*     //   a < 4.0, 4th moment does not exist.   ansi-c*/

   if (_x < 0.0 || _a <= 0.0 || _b <= 0.0)  return (-NEARINFINITY);
/*        //fn_DisplayError("cstz.c/tz_loginversegammapdf(): x, a, and b must be positive");   ansi-c*/

   return ( _a*log(_b) - fn_gammalog(_a) - (_a+1.0)*log(_x) - _b /_x );
}







/*  //<<---------------   ansi-c*/
/*  // P2 algorithm ???????   ansi-c*/
/*  //--------------->>   ansi-c*/
void psqr(double *q, int *m, double x, const double *p, int n)
{
/*     //Outputs:   ansi-c*/
/*     //  q: n-by-1 vector of   ansi-c*/
/*     //  m: n-by-1 vector of   ansi-c*/
/*     //  x: a random draw.   ansi-c*/
/*     //------   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  p:  n-by-1 vector of cumulative cut-off probabilties for the error bands.   ansi-c*/
   static double qm, dq;
   static int i, dm, dn;

   for (i=0; q[i]<=x && i<n; i++) ;
   if (i==0) { q[0]=x; i++; }
   if (i==n) { q[n-1]=x; i--; }
   for (; i<n; i++) m[i]++;
   for (i=1; i<n-1; i++) {
      dq = p[i]*m[n-1];
      if (m[i]+1<=dq && (dm=m[i+1]-m[i])>1) {
         dn = m[i]-m[i-1];
         dq = ((dn+1)*(qm=q[i+1]-q[i])/dm+
            (dm-1)*(q[i]-q[i-1])/dn)/(dm+dn);
         if (qm<dq) dq = qm/dm;
         q[i] += dq;
         m[i]++;
      } else
      if (m[i]-1>=dq && (dm=m[i]-m[i-1])>1) {
         dn = m[i+1]-m[i];
         dq = ((dn+1)*(qm=q[i]-q[i-1])/dm+
            (dm-1)*(q[i+1]-q[i])/dn)/(dm+dn);
         if (qm<dq) dq = qm/dm;
         q[i] -= dq;
         m[i]--;
      }
   }
}
void piksrt(double *arr, int n)
{
/*     //Outputs:   ansi-c*/
/*     //  arr: replaced by new values.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  arr: n-by-1 vector ??????   ansi-c*/
   int i, j;
   double a;

   for (j=1; j<n; j++) {
      a = arr[j];
      for (i=j-1; i>=0 && arr[i]>a; i--)
         arr[i+1] = arr[i];
      arr[i+1]=a;
   }
}



/*  //---------------------------- Some high-level VAR functions ---------------------   ansi-c*/
void fn_lev2growthanual(TSdmatrix *levgro_dm, const TSdmatrix *levgrominus1_dm, const TSivector *indxlogper_iv)
{
/*     // ******* It is the user's responsibility to check memory allocations and dimensions of inputs. *******   ansi-c*/
/*     //Outputs:   ansi-c*/
/*     //  levgro_dm: nfores-by-nvar matrix of annual growth rates (percent) except interest rates and unemployment rate in level.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  levgro_dm: nfores-by-nvar matrix of log levels and, say, interest rates already divided by 100.   ansi-c*/
/*     //  levgrominus1_dm:  qm-by-nvar matrix in the previous year (not necessarily a calendar year).   ansi-c*/
/*     //  indxlogper_iv: nvar-by-1 array of 1, 2, or 4 for the list of endogenous variables.  1: decimal point with annual rate like the interest rate; 2: decimal point (NOT at annual rate) like the unemployment rate; 4: log level value.   ansi-c*/
   int ti, vj, qm, nvar, nfores, totrows;
   TSdmatrix *tf_levgroplus_dm = NULL;

   if ((qm=levgrominus1_dm->nrows) != 12 && qm != 4)  fn_DisplayError("fn_lev2growthanual(): the second input must have 12 or 4 rows for monthly or quarterly data");
   if ((nvar=levgrominus1_dm->ncols) != indxlogper_iv->n || nvar != levgro_dm->ncols)  fn_DisplayError("fn_lev2growthanual(): column dimensions and vector dimension of all inputs must be same");

/*     //=== Memory allocation for this function.   ansi-c*/
   tf_levgroplus_dm = CreateMatrix_lf(qm+(nfores=levgro_dm->nrows), nvar=levgrominus1_dm->ncols);


   CopySubmatrix0(tf_levgroplus_dm, (TSdmatrix *)levgrominus1_dm, 0, 0, qm, nvar);
   CopySubmatrix(tf_levgroplus_dm, qm, 0, levgro_dm, 0, 0, nfores, nvar);
   totrows = qm + nfores;
   for (vj=nvar-1; vj>=0; vj--) {
      switch (indxlogper_iv->v[vj]) {
         case 4:
            for (ti=nfores-1; ti>=0; ti--)
               levgro_dm->M[mos(ti, vj, nfores)] = 100.0*( exp(tf_levgroplus_dm->M[mos(ti+qm, vj, totrows)] - tf_levgroplus_dm->M[mos(ti, vj, totrows)]) - 1.0 );
            break;
         case 2:
         case 1:
            for (ti=nfores-1; ti>=0; ti--)
               levgro_dm->M[mos(ti, vj, nfores)] *= 100.0;
            break;
         default:
            fn_DisplayError("fn_lev2growthanual(): the input vector, indxlogper_iv, must have the integer values 4, 2, and 1");
      }
   }



/*     //=== Destroys memory allocated for this function.   ansi-c*/
   tf_levgroplus_dm = DestroyMatrix_lf(tf_levgroplus_dm);
}



/*  //-------------------   ansi-c*/
/*  // Generating a counterfactual paths conditional on S_T and specified shocks_t(s_t) for _sm (a switching model).   ansi-c*/
/*  //-------------------   ansi-c*/
void fn_ctfals_givenshocks_sm(TSdmatrix *ctfalstran_dm, TSdvector *xprimeminus1_dv, const int bloc, const int eloc, const TSdmatrix *strshockstran_dm,
     const TSivector *S_Tdraw_iv, const TSdcell *Bsdraw_dc, const TSdcell *A0sdrawinv_dc, const TSivector *noshocks_iv)
{
/*     // ******* It is the user's responsibility to check memory allocations and dimensions of inputs. *******   ansi-c*/
/*     //Outputs:  ctflasdrawtran = xprimeminus1*Bsdraw{s} + shocks'*A0sdrawinv{s}.   ansi-c*/
/*     //  ctfalstran_dm: nvar-by-nfores where nfores (=eloc-bloc+1) is the forecast horizon.  Conterfactual paths of nvar variables.   ansi-c*/
/*     //  xprimeminus1_dv:  updated 1-by-ncoef right-hand-side variables at the end of the forecast horizon, ready for the forecasts at the step nfores+1.   ansi-c*/
/*     //    In the order of [nvar for 1st lag, ..., nvar for last lag, other exogenous terms, const term].   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  xprimeminus1_dv: 1-by-ncoef vector of right-hand-side variables at the beginning of the forecast horizon.   ansi-c*/
/*     //  bloc: beginning location for the forecast horizon.   ansi-c*/
/*     //  eloc: end location for the forecast horizon.   ansi-c*/
/*     //  strshockstran_dm: nvar-by-T.  Matrix transpose of unit-variance (time-invariant) structural shocks.   ansi-c*/
/*     //  S_Tdraw_iv: fss-by-1 or SampleSize-by-1 vector of (s_t|I_T,theta).   ansi-c*/
/*     //  Bsdraw_dc:  nStates cells.  For each cell, ncoef-by-nvar reduced-form coefficient matrix.   ansi-c*/
/*     //  A0sdrawinv_dc:  nStates cells.  For each cell, nvar-by-nvar inverse of contemporaneous coefficient matrix.   ansi-c*/
/*     //  noshocks_iv: a (no greater than nvar) vector of base-0 integers indicating the corresponding equations whose shocks are set   ansi-c*/
/*     //               to zero.  Each element of this integer vector must be less than nvar.   ansi-c*/
   int ti, si, vi;
   int nfores = eloc - bloc + 1,
       nvar = ctfalstran_dm->nrows,
       ncoefminusnvar7const = Bsdraw_dc->C[0]->nrows - nvar - 1;
   TSdvector ctfals_sdv, strshocks_sdv;
   TSivector STnfores_siv;   /*  nfores-by-1 vector of s_t's.   ansi-c*/

   if (nfores < 1)  fn_DisplayError("cstz.c/fn_ctfals_givenshocks_sm(): Number of forecast steps must be greater than 0");
   if (eloc > strshockstran_dm->ncols-1)  fn_DisplayError("cstz.c/fn_ctfals_givenshocks_sm(): End location in the forecast horizon must be no greater than the sample size");
   if (nvar != strshockstran_dm->nrows)  fn_DisplayError("cstz.c/fn_ctfals_givenshocks_sm(): the number of rows of strshockstran_dm must be equal to nvar");


/*     // ******* WARNING: The operation involves ctfals_sdv.v, strshocks_sdv.v, STnfores_siv.v  *******   ansi-c*/
/*     // *******          throughout this function is dangerous because of pointer movements.   *******   ansi-c*/
/*     // *******          But it gives us efficiency.                                           *******   ansi-c*/
   ctfals_sdv.n = nvar;
   ctfals_sdv.v = ctfalstran_dm->M;   /*  Points to the beginning of the 1st column of ctfalstran_dm.   ansi-c*/
/*     //+   ansi-c*/
   strshocks_sdv.n = nvar;
   strshocks_sdv.flag = V_DEF;
   strshocks_sdv.v = strshockstran_dm->M + strshockstran_dm->nrows*bloc;   /*  Points to the beginning of the bloc_th column of strshockstran_dm.   ansi-c*/
   for (vi=noshocks_iv->n-1; vi>=0; vi--)
      strshocks_sdv.v[noshocks_iv->v[vi]] = 0.0;    /*  Set shocks in those equations to be zero.   ansi-c*/
/*     //+   ansi-c*/
   STnfores_siv.n = nfores;
   STnfores_siv.flag = V_DEF;
   STnfores_siv.v = S_Tdraw_iv->v + bloc;   /*  Points to the bloc_th position of S_Tdraw_iv.   ansi-c*/


   for (ti=0; ti<nfores; ti++) {
/*        //Must have a forward recursion.   ansi-c*/
      VectorTimesMatrix(&ctfals_sdv, xprimeminus1_dv, Bsdraw_dc->C[si=STnfores_siv.v[ti]], 1.0, 0.0, 'N');
      VectorTimesMatrix(&ctfals_sdv, &strshocks_sdv, A0sdrawinv_dc->C[si], 1.0, 1.0, 'N');
/*        //=== Updates the recursion.  The order matters.   ansi-c*/
      memmove(xprimeminus1_dv->v+nvar, xprimeminus1_dv->v, ncoefminusnvar7const*sizeof(double));
      memcpy(xprimeminus1_dv->v, ctfals_sdv.v, nvar*sizeof(double));
/*        //+   ansi-c*/
      if (ti < nfores-1)   /*  This is needed to prevent memory leak at the end when we have strshocks_sdv.v[noshocks_iv->v[vi]] = 0.0.   ansi-c*/
      {
         ctfals_sdv.v += nvar;   /*  Points to the beginning of the next column of ctfalstran_dm.   ansi-c*/
         strshocks_sdv.v += nvar;   /*  Points to the beginning of the next column of strshockstran_dm.   ansi-c*/
         for (vi=noshocks_iv->n-1; vi>=0; vi--)
            strshocks_sdv.v[noshocks_iv->v[vi]] = 0.0;    /*  Set shocks in those equations to be zero.   ansi-c*/
      }
   }

   ctfalstran_dm->flag = M_GE;
}


/*  //-------------------   ansi-c*/
/*  // Generating a random sequence of counterfactual (ctfal) paths for _sm (a switching model).   ansi-c*/
/*  //-------------------   ansi-c*/
void fn_ctfals_sm(TSdmatrix *ctfalstran_dm, TSdvector *xprimeminus1_dv, const int bloc, const int eloc, const TSdmatrix *strshockstran_dm, const TSivector *Snfores_iv, const TSdcell *Bsdraw_dc, const TSdcell *A0sdrawinv_dc)
{
/*     // ******* It is the user's responsibility to check memory allocations and dimensions of inputs. *******   ansi-c*/
/*     //Outputs:  ctflasdrawtran = xprimeminus1*Bsdraw{s} + shocks'*A0sdrawinv{s}.   ansi-c*/
/*     //  ctfalstran_dm: nvar-by-nfores where nfores (=eloc-bloc+1) is the forecast horizon.  Conterfactual paths of nvar variables.   ansi-c*/
/*     //  xprimeminus1_dv:  updated 1-by-ncoef right-hand-side variables at the end of the forecast horizon, ready for the forecasts at the step nfores+1.   ansi-c*/
/*     //    In the order of [nvar for 1st lag, ..., nvar for last lag, other exogenous terms, const term].   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  xprimeminus1_dv: 1-by-ncoef vector of right-hand-side variables at the beginning of the forecast horizon.   ansi-c*/
/*     //  bloc: beginning location for the forecast horizon.   ansi-c*/
/*     //  eloc: end location for the forecast horizon.   ansi-c*/
/*     //  strshockstran_dm: nvar-by-T.  Matrix transpose of unit-variance (time-invariant) structural shocks.   ansi-c*/
/*     //  Snfores_iv:  nfores-by-1 vector of states where each element is less than nStates.   ansi-c*/
/*     //  Bsdraw_dc:  nStates cells.  For each cell, ncoef-by-nvar reduced-form coefficient matrix.   ansi-c*/
/*     //  A0sdrawinv_dc:  nStates cells.  For each cell, nvar-by-nvar inverse of contemporaneous coefficient matrix.   ansi-c*/
   int ti, si;
   int nfores = eloc - bloc + 1,
       nvar = ctfalstran_dm->nrows,
       ncoefminusnvar7const = Bsdraw_dc->C[0]->nrows - nvar - 1;
   TSdvector ctfals_sdv, strshocks_sdv;

   if (nfores < 1)  fn_DisplayError("cstz.c/fn_ctfals_sm(): Number of forecast steps must be greater than 0");
   if (eloc > strshockstran_dm->ncols-1)  fn_DisplayError("cstz.c/fn_ctfals_sm(): End location in the forecast horizon must be no greater than the sample size");
   if (nvar != strshockstran_dm->nrows)  fn_DisplayError("cstz.c/fn_ctfals_sm(): the number of rows of strshockstran_dm must be equal to nvar");


/*     // ******* WARNING: The operation involves ctfals_sdv.v and strshocks_sdv.v throughout this function *******   ansi-c*/
/*     // *******          is dangerous because of pointer movements.  But it gives us efficiency.          *******   ansi-c*/
   ctfals_sdv.n = nvar;
   ctfals_sdv.v = ctfalstran_dm->M;   /*  Points to the beginning of the 1st column of ctfalstran_dm.   ansi-c*/
   strshocks_sdv.n = nvar;
   strshocks_sdv.flag = V_DEF;
   strshocks_sdv.v = strshockstran_dm->M + strshockstran_dm->nrows*bloc;   /*  Points to the beginning of the bloc_th column of strshockstran_dm.   ansi-c*/


   for (ti=0; ti<nfores; ti++) {
/*        //Must have a forward recursion.   ansi-c*/
      VectorTimesMatrix(&ctfals_sdv, xprimeminus1_dv, Bsdraw_dc->C[si=Snfores_iv->v[ti]], 1.0, 0.0, 'N');
      VectorTimesMatrix(&ctfals_sdv, &strshocks_sdv, A0sdrawinv_dc->C[si], 1.0, 1.0, 'N');
/*        //=== Updates the recursion.  The order matters.   ansi-c*/
      memmove(xprimeminus1_dv->v+nvar, xprimeminus1_dv->v, ncoefminusnvar7const*sizeof(double));
      memcpy(xprimeminus1_dv->v, ctfals_sdv.v, nvar*sizeof(double));
/*        //+   ansi-c*/
      ctfals_sdv.v += nvar;   /*  Points to the beginning of the next column of ctfalstran_dm.   ansi-c*/
      strshocks_sdv.v += nvar;   /*  Points to the beginning of the next column of strshockstran_dm.   ansi-c*/
   }

   ctfalstran_dm->flag = M_GE;
}

/*  //-------------------   ansi-c*/
/*  // Generating a random sequence of counterfactual (ctfal) paths with only monetary policy equation changing to a specified regime while holding other equations' regimes the same as historical ones.   ansi-c*/
/*  //-------------------   ansi-c*/
void fn_ctfals_policyonly(TSdmatrix *ctfalstran_dm, TSdvector *xprimeminus1_dv, const int bloc, const int eloc, const TSdmatrix *strshockstran_dm, const TSivector *S_Tdraw_iv, const int statecon, const int selej, const TSdcell *A0sdraw_dc, const TSdcell *Apsdraw_dc)
{
/*     // ******* It is the user's responsibility to check memory allocations and dimensions of inputs. *******   ansi-c*/
/*     //Outputs:  ctflasdrawtran = xprimeminus1*Bsdraw{s} + shocks'*A0sdrawinv{s}.   ansi-c*/
/*     //  ctfalstran_dm: nvar-by-nfores where nfores (=eloc-bloc+1) is the forecast horizon.  Conterfactual paths of nvar variables.   ansi-c*/
/*     //  xprimeminus1_dv:  updated 1-by-ncoef right-hand-side variables at the end of the forecast horizon, ready for the forecasts at the step nfores+1.   ansi-c*/
/*     //    In the order of [nvar for 1st lag, ..., nvar for last lag, other exogenous terms, const term].   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  xprimeminus1_dv: 1-by-ncoef vector of right-hand-side variables at the beginning of the forecast horizon.   ansi-c*/
/*     //  bloc: beginning location for the forecast horizon.   ansi-c*/
/*     //  eloc: end location for the forecast horizon.   ansi-c*/
/*     //  strshockstran_dm: nvar-by-T.  Matrix transpose of unit-variance (time-invariant) structural shocks.   ansi-c*/
/*     //  S_Tdraw_iv;  fss-by-1 or SampleSize-by-1.  Stores (s_t|I_T,theta).   ansi-c*/
/*     //  statecon: the ith state conditioned for counterfactuals (base 0).  Must be < nStates.   ansi-c*/
/*     //  selej:  location (base 0) of the selected structural equation (e.g., the monetary policy equation).  Only for (1) long-run and short-run responses and (2) counterfactuals with only policy equation at specific state imposed.   ansi-c*/
/*     //  A0sdraw_dc:  nStates cells.  For each cell, nvar-by-nvar contemporaneous coefficient matrix.   ansi-c*/
/*     //  Apsdraw_dc:  nStates cells.  For each cell, ncoef-by-nvar lagged structural coefficient matrix.   ansi-c*/
   int ti, si;
   int errflag = -2,    /*  Initialized to be unsuccessful.  When 0, successful.   ansi-c*/
       nfores = eloc - bloc + 1,
       nvar = ctfalstran_dm->nrows,
       ncoef = Apsdraw_dc->C[0]->nrows,
       nStates = Apsdraw_dc->ncells,
       ncoefminusnvar7const =  ncoef - nvar - 1;
   TSdvector ctfals_sdv, strshocks_sdv;
   TSivector sact_nfores_siv;
/*     //   ansi-c*/
   TSivector *tf_rnstates_iv = CreateConstantVector_int(nStates, nvar),    /*  nStates-by-1: ncoef for each element for *p*_dc or nvar for each elment for *0*_dc.   ansi-c*/
             *tf_cnstates_iv = CreateConstantVector_int(nStates, nvar);    /*  nStates-by-1: nvar for each element for both *p*_dc and *0*_dc.   ansi-c*/
   TSdcell *tf_A0sinv_dc = NULL;
   TSdcell *tf_Aps_dc = NULL,
           *tf_Bs_dc = NULL;



   if (nfores < 1)  fn_DisplayError("cstz.c/fn_ctfals_policyonly(): Number of forecast steps must be greater than 0");
   if (eloc > strshockstran_dm->ncols-1)  fn_DisplayError("cstz.c/fn_ctfals_policyonly(): End location in the forecast horizon must be no greater than the sample size");
   if (nvar != strshockstran_dm->nrows)  fn_DisplayError("cstz.c/fn_ctfals_policyonly(): the number of rows of strshockstran_dm must be equal to nvar");


/*     //=== Memory allocation.   ansi-c*/
   tf_A0sinv_dc = CreateCell_lf(tf_rnstates_iv, tf_cnstates_iv);    /*  Note rnstates_iv and cnstates_iv are already assigned right values.   ansi-c*/
/*     //+   ansi-c*/
   for (si=nStates-1; si>=0; si--)  tf_rnstates_iv->v[si] = ncoef;   /*  Note rnstates_iv is already assigned right values.   ansi-c*/
   tf_Aps_dc = CreateCell_lf(tf_rnstates_iv, tf_cnstates_iv);
   tf_Bs_dc = CreateCell_lf(tf_rnstates_iv, tf_cnstates_iv);


/*     // ******* WARNING: The operation involves ctfals_sdv.v and strshocks_sdv.v throughout this function *******   ansi-c*/
/*     // *******          is dangerous because of pointer movements.  But it gives us efficiency.          *******   ansi-c*/
   ctfals_sdv.n = nvar;
   ctfals_sdv.v = ctfalstran_dm->M;   /*  Points to the beginning of the 1st column of ctfalstran_dm.   ansi-c*/
   strshocks_sdv.n = nvar;
   strshocks_sdv.flag = V_DEF;
   strshocks_sdv.v = strshockstran_dm->M + strshockstran_dm->nrows*bloc;   /*  Points to the beginning of the bloc_th column of strshockstran_dm.   ansi-c*/
/*     //+   ansi-c*/
   sact_nfores_siv.n = nfores;
   sact_nfores_siv.flag = V_DEF;
   sact_nfores_siv.v = S_Tdraw_iv->v + bloc;   /*  Points to the beginning of the bloc_th element of S_Tdraw_iv.   ansi-c*/

/*     //=== Sticks the policy equation at the statecon_th state to A0s and A0p.   ansi-c*/
   for (si=nStates-1; si>=0; si--) {
      CopyMatrix0(tf_A0sinv_dc->C[si], A0sdraw_dc->C[si]);   /*  tf_A0sinv_dc is A0s for a moment.   ansi-c*/
      CopyMatrix0(tf_Aps_dc->C[si], Apsdraw_dc->C[si]);
/*        //=== Sticks the specified regime statecon in the counterfactual period.   ansi-c*/
      CopySubmatrix(tf_A0sinv_dc->C[si], 0, selej, A0sdraw_dc->C[statecon], 0, selej, nvar, 1);
      CopySubmatrix(tf_Aps_dc->C[si], 0, selej, Apsdraw_dc->C[statecon], 0, selej, ncoef, 1);

      if ( errflag=BdivA_rgens(tf_Bs_dc->C[si], tf_Aps_dc->C[si], '/', tf_A0sinv_dc->C[si]) ) {
/*           //tf_A0sinv_dc is at this moment tf_A0s_dc.   ansi-c*/
         printf(".../cstz.c/fn_ctfals_policyonly(): tf_Bs_dc->C[si] -- errors when calling BdivA_rgens() with error flag %d", errflag);
         swzExit(EXIT_FAILURE);
      }
      if ( errflag=invrgen(tf_A0sinv_dc->C[si], tf_A0sinv_dc->C[si]) ) {
         printf(".../cstz.c/fn_ctfals_policyonly(): tf_A0sinv_dc->C -- errors when calling invrgen() with error flag %d", errflag);
         swzExit(EXIT_FAILURE);
      }
   }

   for (ti=0; ti<nfores; ti++) {
/*        //Must have a forward recursion.   ansi-c*/
      VectorTimesMatrix(&ctfals_sdv, xprimeminus1_dv, tf_Bs_dc->C[si=sact_nfores_siv.v[ti]], 1.0, 0.0, 'N');
      VectorTimesMatrix(&ctfals_sdv, &strshocks_sdv, tf_A0sinv_dc->C[si], 1.0, 1.0, 'N');
/*        //=== Updates the recursion.  The order matters.   ansi-c*/
      memmove(xprimeminus1_dv->v+nvar, xprimeminus1_dv->v, ncoefminusnvar7const*sizeof(double));
      memcpy(xprimeminus1_dv->v, ctfals_sdv.v, nvar*sizeof(double));
/*        //+   ansi-c*/
      ctfals_sdv.v += nvar;   /*  Points to the beginning of the next column of ctfalstran_dm.   ansi-c*/
      strshocks_sdv.v += nvar;   /*  Points to the beginning of the next column of strshockstran_dm.   ansi-c*/
   }

   ctfalstran_dm->flag = M_GE;

/*     //=== Destroys memory allocated for this function.   ansi-c*/
   tf_rnstates_iv = DestroyVector_int(tf_rnstates_iv);
   tf_cnstates_iv = DestroyVector_int(tf_cnstates_iv);
   tf_A0sinv_dc = DestroyCell_lf(tf_A0sinv_dc);
   tf_Aps_dc = DestroyCell_lf(tf_Aps_dc);
   tf_Bs_dc = DestroyCell_lf(tf_Bs_dc);
}


#if defined (INTELCMATHLIBRARY)
void fn_impulse(TSdmatrix *imftran_dm, const TSdmatrix *Bh_dm, const TSdmatrix *swishtran_dm, const int nlags, const int imsteps)
{
/*     //Outputs (memory allocated already):   ansi-c*/
/*     //  imftran_dm:  nvar^2-by-imsteps where imf_dm (imsteps-by-nvar^2) is in the same format as in RATS.   ansi-c*/
/*     //             Rows:  nvar responses to the 1st shock, ..., nvar responses to the last shock.   ansi-c*/
/*     //             Columns: steps of impulse responses.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  Bh_dm: ldbh-by-nvar reduced-form coefficient matrix (where ldbh is the leading dimension of Bh_dm and must be at least nvar*nlags) of the form:   ansi-c*/
/*     //           Y(T*nvar) = X*Bh_dm + U, X: T*ldbh(ldbh may include all exogenous terms). Note that columns corresponding equations.   ansi-c*/
/*     //           Columns of Bh_dm: nvar variables for the 1st lag, ..., nvariables for the last lag + (possible exogenous terms) + const = ldbh.   ansi-c*/
/*     //  swishtran_dm: transponse of nvar-by-nvar inv(A0) in the structural model y(t)A0 = e(t).   ansi-c*/
/*     //  nlags:  lag length (number of lags);   ansi-c*/
/*     //  imsteps:  steps for impulse responses.   ansi-c*/

   int i, j,
       nvar, nvar2, ldbh, jmax;
   double *Bh, *imftran;

   if (!imftran_dm)  fn_DisplayError(".../fn_impulse():  the output impulse matrix imftran_dm must be created (memory-allocated)");
   else if (!Bh_dm || !swishtran_dm)  fn_DisplayError(".../fn_impulse():  the input matrices Bh_dm and swich_dm must be created (memory-allocated)");
   else if (!Bh_dm->flag || !swishtran_dm->flag)   fn_DisplayError(".../fn_impulse():  the input matrices Bh_dm and swich_dm must be given legal values");
   else if (nlags < 1)  fn_DisplayError(".../fn_impulse():  the lag length, nlags, must be equal to or greater than 1");
   else if (imsteps <1)  fn_DisplayError(".../fn_impulse():  the number of steps for impulse responses, imsteps, must be must be equal to or greater than 1");
   else if ((nvar = swishtran_dm->nrows) != swishtran_dm->ncols )  fn_DisplayError(".../fn_impulse():  the input matrix, swishtran_dm, must be square");
   else if (nvar != Bh_dm->ncols)  fn_DisplayError(".../fn_impulse():  the number of columns in Bh_dm must equal to the number of equations or endogenous variables");
   else if (square(nvar) != imftran_dm->nrows || imsteps != imftran_dm->ncols)  fn_DisplayError(".../fn_impulse():  Dimension of impulse matrix input matrix imftran_dm is incompatible with other input matrices or with the number of steps");

/*     //if ( !(imftran_dm->flag & M_CN) && imftran_dm[0] !=0.0 )  InitializeConstantMatrix_lf(imftran_dm, 0.0);   ansi-c*/
   InitializeConstantMatrix_lf(imftran_dm, 0.0);   /*  Cumulative. Always initialize it to zero.   ansi-c*/


   nvar2 = square(nvar);
   Bh = Bh_dm->M;
   imftran = imftran_dm->M;


   if ((ldbh=Bh_dm->nrows) < nvar*nlags)  fn_DisplayError("Input matrix Bh_dm must have at least nvar*nlags rows");
   cblas_dcopy(nvar2, swishtran_dm->M, 1, imftran, 1);
   for (i=1; i<imsteps; i++) {
      jmax = i<nlags?i:nlags;
      for (j=0; j<jmax; j++) {
         cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, nvar, nvar, nvar,
            1.0, &Bh[j*nvar], ldbh, &imftran[(i-j-1)*nvar2], nvar,
            1.0, &imftran[i*nvar2], nvar);
      }
   }


   imftran_dm->flag = M_GE;
}
#else
/*  //No default routine yet.  7 Oct 2003   ansi-c*/
#endif


TSdmatrix *tz_impulse2levels(TSdmatrix *imflev_dm, TSdmatrix *imf_dm, TSivector *vlist2levels_iv)
{
/*     //Converting imf_dm to the level impulse responses imflev_dm according to vlist2levels_iv.   ansi-c*/
/*     //If imflev_dm = imf_dm, then the value of imf_dm will be replaced by the new value.   ansi-c*/
/*     //   ansi-c*/
/*     //imf_dm; nsteps-by-nvar^2 where   ansi-c*/
/*     //          rows: steps of impulse responses;   ansi-c*/
/*     //          columns:  nvar responses to the 1st shock, ..., nvar responses to the last shock.   ansi-c*/
/*     //vlist2levels_iv;  must be in ascending order.  A list of base-0 variables to be converted to levels.  Example: [0 1 3]   ansi-c*/
   int _i, _j, _t;
   int largestvar;    /*  last variable corresponding to the largest number.   ansi-c*/
   int _n, nsq, imsteps;
   TSdvector imf_sdv;
   TSdvector imflev_sdv;

   if (!imf_dm || !imf_dm->flag)
      fn_DisplayError(".../cstz.c/tz_impulse2levels(): the input matrix imf_dm must be (1) allocated memory and (2) given legal values");

   if (!imflev_dm) {
      imflev_dm = CreateMatrix_lf(imf_dm->nrows, imf_dm->ncols);
      imflev_dm->flag = M_GE;    /*  Legal values will be given below.   ansi-c*/
   }
   else if (imflev_dm != imf_dm )
      if ( (imflev_dm->nrows != imf_dm->nrows) || (imflev_dm->ncols != imf_dm->ncols))
         fn_DisplayError(".../cstz.c/tz_impulse2levels(): dimensions of the input matrix imf_dm and the output matrix imflev_dm must match exactly");
      else  imflev_dm->flag = M_GE;    /*  Legal values will be given below.   ansi-c*/

   largestvar = vlist2levels_iv->v[vlist2levels_iv->n-1]+1;
   _n = (int)floor(sqrt(imf_dm->ncols)+0.5);
   nsq = imf_dm->ncols;
   if ( square(largestvar) > nsq)
      fn_DisplayError(".../cstz.c/tz_impulse2levels():  the last specified variable in vlist2levels_iv is out of the range of impulse responses");


   imflev_sdv.n = imf_sdv.n = imf_dm->nrows;
   imflev_sdv.flag = imf_sdv.flag = V_DEF;   /*  Legal values will be given below.   ansi-c*/
   imsteps = imf_dm->nrows;
   for (_i=vlist2levels_iv->n-1; _i>=0; _i--)
      for (_j=vlist2levels_iv->v[_i]; _j<nsq; _j += _n) {
         imflev_sdv.v = imflev_dm->M + _j*imsteps;
         imf_sdv.v = imf_dm->M + _j*imsteps;
         imflev_sdv.v[0] = imf_sdv.v[0];
         for (_t=1; _t<imsteps; _t++)
            imflev_sdv.v[_t] = imflev_sdv.v[_t-1] + imf_sdv.v[_t];
      }

   return (imflev_dm);
}


void DynamicResponsesForStructuralEquation(TSdmatrix *Resps_dm, const int loclv, const int nlags, const TSdvector *a0p_dv)
{
/*     //Outputs:   ansi-c*/
/*     //  Resps_dm: k-by-nvar where k responses of the loclv_th variable to the _ith variable for _i=1:nvar.   ansi-c*/
/*     //    The loclv_th column of Resps_dm is meaningless but as a debug check should be close to -1 for the kth responses as k->\infty.   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  loclv:  loction of the left-hand variable either in difference (growth) or level.   ansi-c*/
/*     //  nlags:  number of lags.   ansi-c*/
/*     //  a0p_dv: m-by-1 vector of [a0 a+] either in difference (growth) or level for the strctural equation considered where m>= (nlags+1)*nvar because m may   ansi-c*/
/*     //    include the constant term.  Note a0 is on the left hand side of the equation and a+ is on the right hand side of the equation.   ansi-c*/
   int vi, li;
   int nvar, K;
   double tmpdsum, c0, a0inv;
   TSdvector resps_sdv;   /*  k-by-1.   ansi-c*/
/*     //----   ansi-c*/
   TSdvector *a1_dv = NULL;      /*  nlags-by-1.   ansi-c*/

   if (!Resps_dm || !a0p_dv || !a0p_dv->flag)    fn_DisplayError(".../cstz/DynamicResponsesForStructuralEquation():  (1) both input vector and output matrix must be allocated memory; (2) the input vector must have legal values");
   if (a0p_dv->n < (nlags+1)*(nvar=Resps_dm->ncols))  fn_DisplayError(".../cstz/DynamicResponsesForStructuralEquation(): the length of the input vector must be at least (nvar+1)*nlags");
   if (loclv >= nvar || loclv < 0)  fn_DisplayError(".../cstz/DynamicResponsesForStructuralEquation():  the location for the left-hand-side variable must be between 0 and number of variables-1, inclusive");
   a1_dv = CreateVector_lf(nlags);
   a1_dv->flag = V_DEF;   /*  which will be given legal values below.   ansi-c*/

   resps_sdv.n = K = Resps_dm->nrows;
   resps_sdv.flag = V_UNDEF;

   a0inv = 1.0/a0p_dv->v[loclv];
   for (li=nlags; li>=1; li--)       /*  Note li=1; li<=nlags, NOT li=0; li<nlags.   ansi-c*/
      a1_dv->v[li-1] = a0p_dv->v[loclv+nvar*li]*a0inv;
/*              //Constructing the lagged coefficients for the loclv_th variable.   ansi-c*/
   for (vi=nvar-1; vi>=0; vi--) {
/*        //=== Constructing the constant term.   ansi-c*/
      tmpdsum = - a0p_dv->v[vi];   /*  Assigned to -a_0.   ansi-c*/
      for (li=nlags; li>=1; li--)        /*  Note li=1; li<=nlags, NOT li=0; li<nlags.   ansi-c*/
                  tmpdsum += a0p_dv->v[vi+nvar*li];
      c0 = tmpdsum*a0inv;
/*               //Done with t* array.   ansi-c*/

/*        //=== Getting dynamic responses to the vi_th variable.   ansi-c*/
      resps_sdv.v = Resps_dm->M + vi*K;
      DynamicResponsesAR(&resps_sdv, c0, a1_dv);
   }
   Resps_dm->flag = M_GE;


/*     //=== Destroys memory allocated for this function only.   ansi-c*/
   a1_dv = DestroyVector_lf(a1_dv);
}



void DynamicResponsesAR(TSdvector *resps_dv, const double c0, const TSdvector *a1_dv)
{
/*     //Outputs:   ansi-c*/
/*     //  resps_dv: k-by-1 where k responses r_{t+1} to r_{t+k} are computed from r_{t+1} = c0 + a1'*[r_t; ...; r_{t-nlags+1}].   ansi-c*/
/*     //Inputs:   ansi-c*/
/*     //  c0:  constant term.   ansi-c*/
/*     //  a1_dv: nlags-by-1 vector of coefficients in the AR process.   ansi-c*/
   int ti;
   int k, nlags;
   double *rv;
   TSdvector *rlags_dv = NULL;

   if (!resps_dv || !a1_dv || !a1_dv->flag)  fn_DisplayError(".../cstz/DynamicResponsesAR():  (1) both input and output vectors must be allocated memory; (2) the input vector must have legal values");
   rlags_dv = CreateConstantVector_lf(nlags=a1_dv->n, 0.0);

   rv = resps_dv->v;
   k = resps_dv->n;

   *(rlags_dv->v) = *rv = c0;


   for (ti=1; ti<k; ti++) {
/*        //Note ti=1, NOT ti=0.   ansi-c*/
      rv[ti] = c0 + VectorDotVector((TSdvector *)a1_dv, rlags_dv);
/*        //=== Updating rlags_dv.   ansi-c*/
      memmove(rlags_dv->v+1, rlags_dv->v, (nlags-1)*sizeof(double));
      *(rlags_dv->v) = rv[ti];
   }
   resps_dv->flag = V_DEF;

/*     //=== Destroys memory allocated for this function only.   ansi-c*/
   rlags_dv = DestroyVector_lf(rlags_dv);
}





/*  //---------------------------- Some regular vector or matrix operations ---------------------   ansi-c*/
double MinVector_lf(TSdvector *x_dv) {
/*     //Input: no change for x_dv in this function.   ansi-c*/
   int _i, n;
   double minvalue;
   double *v;

   if (!x_dv || !x_dv->flag) fn_DisplayError(".../cstz.c/MinVector_lf():  Input vector x_dv must be (1) allocated memory and (2) assigned legal values");
   n = x_dv->n;
   v = x_dv->v;

   minvalue = v[0];
   for (_i=n-1; _i>0; _i--)
      if (v[_i]<minvalue)  minvalue = v[_i];

   return( minvalue );
}

TSdvector *ConvertVector2exp(TSdvector *y_dv, TSdvector *x_dv)
{
/*     //y=exp(x): output vector.  If NULL, y will be created and memory-allocated.   ansi-c*/
/*     //x: input vector.   ansi-c*/
   TSdvector *z_dv=NULL;
   #if defined (INTELCMATHLIBRARY)
   int _i;
   #endif


   if (!x_dv || !x_dv->flag)  fn_DisplayError(".../cstz.c/ConvertVector2exp(): input vector must be (1) created and (2) given legal values");

   #if !defined (INTELCMATHLIBRARY)

   if (!y_dv)
   {
      z_dv = CreateVector_lf(x_dv->n);
      vdExp(x_dv->n, x_dv->v, z_dv->v);
      z_dv->flag = V_DEF;
      return (z_dv);
   }
   else if (x_dv!=y_dv)
   {
      vdExp(x_dv->n, x_dv->v, y_dv->v);
      y_dv->flag = V_DEF;
      return (y_dv);
   }
   else
   {
      z_dv = CreateVector_lf(x_dv->n);
      vdExp(x_dv->n, x_dv->v, z_dv->v);
      z_dv->flag = V_DEF;
      CopyVector0(x_dv, z_dv);
      DestroyVector_lf(z_dv);
      return (x_dv);
   }

   #else

   if (!y_dv)  z_dv = CreateVector_lf(x_dv->n);
   else  z_dv = y_dv;
   for (_i=x_dv->n-1; _i>=0; _i--)  z_dv->v[_i] = exp(x_dv->v[_i]);
   z_dv->flag = V_DEF;
   return (z_dv);

   #endif
}
/*  //---   ansi-c*/
TSdvector *ConvertVector2log(TSdvector *y_dv, TSdvector *x_dv)
{
/*     //y=log(x): output vector.  If NULL, y will be created and memory-allocated.   ansi-c*/
/*     //x: input vector.   ansi-c*/
   TSdvector *z_dv=NULL;
   #if defined (INTELCMATHLIBRARY)
   int _i;
   #endif


   if (!x_dv || !x_dv->flag)  fn_DisplayError(".../cstz.c/ConvertVector2exp(): input vector must be (1) created and (2) given legal values");

   #if !defined (INTELCMATHLIBRARY)

   if (!y_dv)
   {
      z_dv = CreateVector_lf(x_dv->n);
      vdLn(x_dv->n, x_dv->v, z_dv->v);
      z_dv->flag = V_DEF;
      return (z_dv);
   }
   else if (x_dv!=y_dv)
   {
      vdLn(x_dv->n, x_dv->v, y_dv->v);
      y_dv->flag = V_DEF;
      return (y_dv);
   }
   else
   {
      z_dv = CreateVector_lf(x_dv->n);
      vdLn(x_dv->n, x_dv->v, z_dv->v);
      z_dv->flag = V_DEF;
      CopyVector0(x_dv, z_dv);
      DestroyVector_lf(z_dv);
      return (x_dv);
   }

   #else

   if (!y_dv)  z_dv = CreateVector_lf(x_dv->n);
   else  z_dv = y_dv;
   for (_i=x_dv->n-1; _i>=0; _i--)  z_dv->v[_i] = log(x_dv->v[_i]);
   z_dv->flag = V_DEF;
   return (z_dv);

   #endif
}

double tz_normofvector(TSdvector *x_dv, double p)
{
   double norm = 0.0;
   int ki, _n;
   double *v;

   if ( !x_dv || !x_dv->flag )  fn_DisplayError("/cstz.c/tz_normofvector(): Input x_dv must have (1) memory and (2) legal values");
   if (p<1.0)  fn_DisplayError("/cstz.c/tz_normofvector(): The input p must be no less than 1.0");
   _n = x_dv->n;
   v = x_dv->v;

   if (p==2.0)
   {
      for (ki=_n-1; ki>=0; ki--)  norm += v[ki]*v[ki];
      norm = sqrt(norm);
   }
   else
   {
      printf("\n/cstz.c/tz_normofvector(): HELLO I TRICK YOU and YOU MUST DO fabs(p-2.0)<MICHINEZERO!!!!!!\n");  /*  ????   ansi-c*/
      if (p==1.0)
         for (ki=_n-1; ki>=0; ki--)  norm += fabs(v[ki]);
      else
      {
         for (ki=_n-1; ki>=0; ki--)  norm += pow(fabs(v[ki]), p);
         norm = pow(norm, 1.0/p);
      }
   }

   return (norm);
}



/*  //---------------------------- Not used often ---------------------   ansi-c*/
void fn_cumsum(double **aos_v, int *aods_v, double *v, int d_v) {
/*     // Compute a cumulative sum of a vector.   ansi-c*/
/*     //   ansi-c*/
/*     // v: an n-by-1 vector.   ansi-c*/
/*     // d_v: n -- size of the vector v to be used for a cumulative sum.   ansi-c*/
/*     // aos_v: address of the pointer to the n-by-1 vector s_v.   ansi-c*/
/*     // aods_v: address of the size of the dimension of s_v.   ansi-c*/
/*     //----------   ansi-c*/
/*     // *aos_v: An n-by-1 vector of cumulative sum s_v.   ansi-c*/
/*     // *aods_v: n -- size of the dimension for s_v.   ansi-c*/

   int ki;

   *aos_v = tzMalloc(d_v, double);
   (*aods_v) = d_v;                                /*   n for the n-by-1 vector s_v.   ansi-c*/
   *(*aos_v) = *v;
   if (d_v>1) {
      for (ki=1; ki<d_v; ki++) (*aos_v)[ki] = (*aos_v)[ki-1] + v[ki];
   }
}



/**
void fn_ergodp(double **aop, int *aod, mxArray *cp) {
   // Compute the ergodic probabilities.  See Hamilton p.681.
   //
   // cp: n-by-n Markovian transition matrix.
   // aop: address of the pointer to the n-by-1 vector p.
   // aod: address of the size of the dimension of p.
   //----------
   // *aop: n-by-1 vector of ergodic probabilities p.  @@Must be freed outside this function.@@
   // *aod: n -- size of the dimension for p (automatically supplied within this function).

   mxArray *gpim=NULL, *gpid=NULL;   // m: n-by-n eigvector matrix; d: n-by-n eigvalue diagonal.
   double *gpim_p, *gpid_p;             // _p:  a pointer to the corresponding mxArray whose name occurs before _p.
         //------- Note the following two lines will cause Matlab or C to crash because gpim has not been initialized so it points to garbage.
         //   double *gpim_p = mxGetPr(gpim);
         //   double *gpid_p = mxGetPr(gpid);
   int eigmaxindx,                      // Index of the column corresponding to the max eigenvalue.
       n, ki;
   double gpisum=0.0,
          eigmax, tmpd0;

   n=mxGetM(cp);                        // Get n for the n-by-n mxArray cp.
   (*aod)=n;

   *aop = tzMalloc(n, double);

   gpim = mlfEig(&gpid,cp,NULL,NULL);
   gpim_p = mxGetPr(gpim);
   gpid_p = mxGetPr(gpid);

   eigmax = *gpid_p;
   eigmaxindx = 0;
   if (n>1) {
      for (ki=1;ki<n;ki++) {
         if (gpid_p[n*ki+ki] > eigmax) {
            eigmax=gpid_p[n*ki+ki];
            eigmaxindx=ki;
         }                           // Note that n*ki+ki refers to a diagonal location in the n-by-n matrix.
      }
   }
   for (ki=0;ki<n;ki++) {
      gpisum += gpim_p[n*eigmaxindx+ki];                 // Sum over the eigmaxindx_th column.
   }
   tmpd0 = 1.0/gpisum;
   for (ki=0;ki<n;ki++) {
      (*aop)[ki] = gpim_p[n*eigmaxindx+ki]*tmpd0;                 // Normalized eigmaxindx_th column as ergodic probabilities.
   }

   mxDestroyArray(gpim);                // ????? swzFree(gpim_p)
   mxDestroyArray(gpid);
}
/**/



/*  //---------- Must keep the following code forever. ---------------   ansi-c*/
/**
TSdp2m5 *CreateP2m5(const double p)
{
   TSdp2m5 *x_dp2m5 = tzMalloc(1, TSdp2m5);

   if (p<=0.0 && p>=1.0)  fn_DisplayError(".../cstz.c/CreateP2m5_lf():  input probability p must be between 0.0 and 1.0");

   x_dp2m5->cnt = 0;
   x_dp2m5->ndeg = 0;
   x_dp2m5->p = tzMalloc(5, double);
   x_dp2m5->q = tzMalloc(5, double);
   x_dp2m5->m = tzMalloc(5, int);

   x_dp2m5->p[0] = 0.00;
   x_dp2m5->p[1] = 0.5*p;
   x_dp2m5->p[2] = p;
   x_dp2m5->p[3] = 0.5*(1.0+p);
   x_dp2m5->p[4] = 1.00;

   return (x_dp2m5);
}
TSdp2m5 *DestroyP2m5(TSdp2m5 *x_dp2m5)
{
   if (x_dp2m5) {
      swzFree(x_dp2m5->m);
      swzFree(x_dp2m5->q);
      swzFree(x_dp2m5->p);

      swzFree(x_dp2m5);
      return ((TSdp2m5 *)NULL);
   }
   else  return (x_dp2m5);
}
TSdvectorp2m5 *CreateVectorP2m5(const int n, const double p)
{
   int _i;
   //
   TSdvectorp2m5 *x_dvp2m5 = tzMalloc(1, TSdvectorp2m5);

   x_dvp2m5->n = n;
   x_dvp2m5->v = tzMalloc(n, TSdp2m5 *);
   for (_i=n-1; _i>=0; _i--)
      x_dvp2m5->v[_i] = CreateP2m5(p);

   return (x_dvp2m5);
}
TSdvectorp2m5 *DestroyVectorP2m5(TSdvectorp2m5 *x_dvp2m5)
{
   int _i;

   if (x_dvp2m5) {
      for (_i=x_dvp2m5->n-1; _i>=0; _i--)
         x_dvp2m5->v[_i] = DestroyP2m5(x_dvp2m5->v[_i]);
      swzFree(x_dvp2m5->v);

      swzFree(x_dvp2m5);
      return ((TSdvectorp2m5 *)NULL);
   }
   return  (x_dvp2m5);
}
TSdmatrixp2m5 *CreateMatrixP2m5(const int nrows, const int ncols, const double p)
{
   int _i;
   //
   TSdmatrixp2m5 *X_dmp2m5 = tzMalloc(1, TSdmatrixp2m5);

   X_dmp2m5->nrows = nrows;
   X_dmp2m5->ncols = ncols;
   X_dmp2m5->M = tzMalloc(nrows*ncols, TSdp2m5 *);
   for (_i=nrows*ncols-1; _i>=0; _i--)
      X_dmp2m5->M[_i] = CreateP2m5(p);

   return (X_dmp2m5);
}
TSdmatrixp2m5 *DestroyMatrixP2m5(TSdmatrixp2m5 *X_dmp2m5)
{
   int _i;

   if (X_dmp2m5) {
      for (_i=X_dmp2m5->nrows*X_dmp2m5->ncols-1; _i>=0; _i--)
         X_dmp2m5->M[_i] = DestroyP2m5(X_dmp2m5->M[_i]);
      swzFree(X_dmp2m5->M);

      swzFree(X_dmp2m5);
      return ((TSdmatrixp2m5 *)NULL);
   }
   else  return (X_dmp2m5);
}
TSdcellp2m5 *CreateCellP2m5(const TSivector *rows_iv, const TSivector *cols_iv, const double p)
{
   int _i;
   int ncells;
   //
   TSdcellp2m5 *X_dcp2m5 = tzMalloc(1, TSdcellp2m5);


   if (!rows_iv || !cols_iv || !rows_iv->flag || !cols_iv->flag)  fn_DisplayError(".../cstz.c/CreateCellP2m5(): Input row and column vectors must be (1) created and (2) assigned legal values");
   if ((ncells=rows_iv->n) != cols_iv->n)  fn_DisplayError(".../cstz.c/CreateCellP2m5(): Length of rows_iv must be the same as that of cols_iv");


   X_dcp2m5->ncells = ncells;
   X_dcp2m5->C = tzMalloc(ncells, TSdmatrixp2m5 *);
   for (_i=ncells-1; _i>=0; _i--)
      X_dcp2m5->C[_i] = CreateMatrixP2m5(rows_iv->v[_i], cols_iv->v[_i], p);

   return (X_dcp2m5);
}
TSdcellp2m5 *DestroyCellP2m5(TSdcellp2m5 *X_dcp2m5)
{
   int _i;

   if (X_dcp2m5) {
      for (_i=X_dcp2m5->ncells-1; _i>=0; _i--)
         X_dcp2m5->C[_i] = DestroyMatrixP2m5(X_dcp2m5->C[_i]);
      swzFree(X_dcp2m5->C);

      swzFree(X_dcp2m5);
      return ((TSdcellp2m5 *)NULL);
   }
   else  return (X_dcp2m5);
}


#define P2REALBOUND DBL_MAX
int P2m5Update(TSdp2m5 *x_dp2m5, const double newval)
{
   //5-marker P2 algorithm.
   //quantiles q[0] to q[4] correspond to 5-marker probabilities {0.0, p/5, p, (1+p)/5, 1.0}.
   //Outputs:
   //  x_dp2m5->q, the markers x_dp2m5->m, is updated and only x_dp2m5->q[2] is used.
   //Inputs:
   //  newval: new random number.
   //
   // January 2003.
   int k, j;
   double a;
   double qm, dq;
   int i, dm, dn;


   if (!x_dp2m5)  fn_DisplayError(".../cstz.c/P2m5Update(): x_dp2m5 must be created");

   //if (isgreater(newval, -P2REALBOUND) && isless(newval, P2REALBOUND)) {
   if (isfinite(newval) && newval > -P2REALBOUND && newval < P2REALBOUND) {
      if (++x_dp2m5->cnt > 5) {
         //Updating the quantiles and markers.
         for (i=0; x_dp2m5->q[i]<=newval && i<5; i++) ;
         if (i==0) { x_dp2m5->q[0]=newval; i++; }
         if (i==5) { x_dp2m5->q[4]=newval; i--; }
         for (; i<5; i++) x_dp2m5->m[i]++;
         for (i=1; i<4; i++) {
            dq = x_dp2m5->p[i]*x_dp2m5->m[4];
            if (x_dp2m5->m[i]+1<=dq && (dm=x_dp2m5->m[i+1]-x_dp2m5->m[i])>1) {
               dn = x_dp2m5->m[i]-x_dp2m5->m[i-1];
               dq = ((dn+1)*(qm=x_dp2m5->q[i+1]-x_dp2m5->q[i])/dm+
                  (dm-1)*(x_dp2m5->q[i]-x_dp2m5->q[i-1])/dn)/(dm+dn);
               if (qm<dq) dq = qm/dm;
               x_dp2m5->q[i] += dq;
               x_dp2m5->m[i]++;
            } else
            if (x_dp2m5->m[i]-1>=dq && (dm=x_dp2m5->m[i]-x_dp2m5->m[i-1])>1) {
               dn = x_dp2m5->m[i+1]-x_dp2m5->m[i];
               dq = ((dn+1)*(qm=x_dp2m5->q[i]-x_dp2m5->q[i-1])/dm+
                  (dm-1)*(x_dp2m5->q[i+1]-x_dp2m5->q[i])/dn)/(dm+dn);
               if (qm<dq) dq = qm/dm;
               x_dp2m5->q[i] -= dq;
               x_dp2m5->m[i]--;
            }
         }
      }
      else if (x_dp2m5->cnt < 5) {
         //Fills the initial values.
         x_dp2m5->q[x_dp2m5->cnt-1] = newval;
         x_dp2m5->m[x_dp2m5->cnt-1] = x_dp2m5->cnt-1;
      }
      else {
         //=== Last filling of initial values.
         x_dp2m5->q[4] = newval;
         x_dp2m5->m[4] = 4;
         //=== P2 algorithm begins with reshuffling quantiles and makers.
         for (j=1; j<5; j++) {
            a = x_dp2m5->q[j];
            for (k=j-1; k>=0 && x_dp2m5->q[k]>a; k--)
               x_dp2m5->q[k+1] = x_dp2m5->q[k];
            x_dp2m5->q[k+1]=a;
         }
      }
   }
   else  ++x_dp2m5->ndeg;  //Throwing away the draws to treat exceptions.

   return (x_dp2m5->cnt);
}
#undef P2REALBOUND

void P2m5MatrixUpdate(TSdmatrixp2m5 *X_dmp2m5, const TSdmatrix *newval_dm)
{
   int _i;
   int nrows, ncols;

   if (!X_dmp2m5 || !newval_dm || !newval_dm->flag)  fn_DisplayError(".../cstz.c/P2m5MatrixUpdate():  (1) Matrix struct X_dmp2m5 must be created and (2) input new value matrix must be crated and given legal values");
   if ((nrows=newval_dm->nrows) != X_dmp2m5->nrows || (ncols=newval_dm->ncols) != X_dmp2m5->ncols)
      fn_DisplayError(".../cstz.c/P2m5MatrixUpdate(): Number of rows and colums in X_dmp2m5 must match those of newval_dm");

   for (_i=nrows*ncols-1; _i>=0; _i--)
      P2m5Update(X_dmp2m5->M[_i], newval_dm->M[_i]);
}

void P2m5CellUpdate(TSdcellp2m5 *X_dcp2m5, const TSdcell *newval_dc)
{
   int _i;
   int ncells;

   if (!X_dcp2m5 || !newval_dc)  fn_DisplayError(".../cstz.c/P2m5CellUpdate():  (1) Cell struct X_dcp2m5 must be created and (2) input new value cell must be crated and given legal values");
   if ((ncells=newval_dc->ncells) != X_dcp2m5->ncells)
      fn_DisplayError(".../cstz.c/P2m5MatrixUpdate(): Number of cells in X_dcp2m5 must match that of newval_dc");

   for (_i=ncells-1-1; _i>=0; _i--)
      P2m5MatrixUpdate(X_dcp2m5->C[_i], newval_dc->C[_i]);
}
/**/