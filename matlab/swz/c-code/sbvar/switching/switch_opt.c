
#include "switch_opt.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "modify_for_mex.h"

/*  //====== Static Global Variables ======   ansi-c*/
static TStateModel *Model=(TStateModel*)NULL;
static PRECISION *buffer=(PRECISION*)NULL;
static PRECISION *ModifiedFreeParameters=(PRECISION*)NULL;
static PRECISION *FreeParameters_Q=(PRECISION*)NULL;
static int NumberFreeParameters_Q=0;
static PRECISION *FreeParameters_Theta=(PRECISION*)NULL;
static int NumberFreeParameters_Theta=0;


void SetupObjectiveFunction(TStateModel *model, PRECISION *Modified, PRECISION *FreeQ, PRECISION *FreeTheta)
{
  if (buffer) swzFree(buffer);
  Model=model;
  FreeParameters_Q=FreeQ;
  NumberFreeParameters_Q=NumberFreeParametersQ(model);
  FreeParameters_Theta=FreeTheta;
  NumberFreeParameters_Theta=model->routines->pNumberFreeParametersTheta(model);
  ModifiedFreeParameters=Modified;
}

void SetupObjectiveFunction_new(TStateModel *model, int FreeTheta_Idx, int FreeQ_Idx, int Modified_Idx)
{
  if (buffer) swzFree(buffer);
  Model=model;
  NumberFreeParameters_Q=NumberFreeParametersQ(model);
  NumberFreeParameters_Theta=model->routines->pNumberFreeParametersTheta(model);
  buffer=(PRECISION*)swzMalloc((NumberFreeParameters_Q + NumberFreeParameters_Theta)*sizeof(PRECISION));

  FreeParameters_Q=buffer+FreeQ_Idx;
  FreeParameters_Theta=buffer+FreeTheta_Idx;
  ModifiedFreeParameters=buffer+Modified_Idx;
}

PRECISION PosteriorObjectiveFunction(PRECISION *x, int n)
{
  if (x != ModifiedFreeParameters) memmove(ModifiedFreeParameters,x,n*sizeof(PRECISION));
  ConvertFreeParametersToQ(Model,FreeParameters_Q);
  ConvertFreeParametersToTheta(Model,FreeParameters_Theta);
  return -LogPosterior_StatesIntegratedOut(Model);

/*    //PRECISION lp_Q, lp_Theta, li;   ansi-c*/
/*    //FILE *f_out;   ansi-c*/
/*    //lp_Q=LogPrior_Q(Model);   ansi-c*/
/*    //lp_Theta=LogPrior_Theta(Model);   ansi-c*/
/*    //li=LogLikelihood_StatesIntegratedOut(Model);   ansi-c*/
/*    //if (isnan(lp_Q) || isnan(lp_Theta) || isnan(li))   ansi-c*/
/*    //  {   ansi-c*/
/*    //    f_out=fopen("tmp.tmp","wt");   ansi-c*/
/*    //    Write_VAR_Specification(f_out,(char*)NULL,Model);   ansi-c*/
/*    //    WriteTransitionMatrices(f_out,(char*)NULL,"Error: ",Model);   ansi-c*/
/*    //    Write_VAR_Parameters(f_out,(char*)NULL,"Error: ",Model);   ansi-c*/
/*    //    fprintf(f_out,"LogPrior_Theta(): %le\n",lp_Theta);   ansi-c*/
/*    //    fprintf(f_out,"LogPrior_Q(): %le\n",lp_Q);   ansi-c*/
/*    //    fprintf(f_out,"LogLikelihood_StatesIntegratedOut(): %le\n",li);   ansi-c*/
/*    //    fprintf(f_out,"Posterior: %le\n\n",lp_Q+lp_Theta+li);   ansi-c*/
/*    //    fclose(f_out);   ansi-c*/
/*    //    swzExit(0);   ansi-c*/
/*    //  }   ansi-c*/
/*    //return -(lp_Q+lp_Theta+li);   ansi-c*/
}

PRECISION PosteriorObjectiveFunction_csminwel(double *x, int n, double **args, int *dims)
{
  return PosteriorObjectiveFunction(x,n);
}

void PosteriorObjectiveFunction_npsol(int *mode, int *n, double *x, double *f, double *g, int *nstate)
{
  *f=PosteriorObjectiveFunction(x,*n);
}

PRECISION MLEObjectiveFunction(PRECISION *x, int n)
{
  if (x != ModifiedFreeParameters) memmove(ModifiedFreeParameters,x,n*sizeof(PRECISION));
  ConvertFreeParametersToQ(Model,FreeParameters_Q);
  ConvertFreeParametersToTheta(Model,FreeParameters_Theta);
  return -LogLikelihood_StatesIntegratedOut(Model);
}

PRECISION MLEObjectiveFunction_csminwel(double *x, int n, double **args, int *dims)
{
  return MLEObjectiveFunction(x,n);
}

void MLEObjectiveFunction_npsol(int *mode, int *n, double *x, double *f, double *g, int *nstate)
{
  *f=MLEObjectiveFunction(x,*n);
}


PRECISION MLEObjectiveFunction_LogQ(PRECISION *x, int n)
{
  if (x != ModifiedFreeParameters) memmove(ModifiedFreeParameters,x,n*sizeof(PRECISION));
  ConvertLogFreeParametersToQ(Model,FreeParameters_Q);
  ConvertFreeParametersToTheta(Model,FreeParameters_Theta);
  return -LogLikelihood_StatesIntegratedOut(Model);
}

PRECISION MLEObjectiveFunction_LogQ_csminwel(double *x, int n, double **args, int *dims)
{
  return MLEObjectiveFunction_LogQ(x,n);
}




