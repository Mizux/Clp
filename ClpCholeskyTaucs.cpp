#ifdef REAL_BARRIER
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.



#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "ClpHelperFunctions.hpp"

#include "ClpInterior.hpp"
#include "ClpCholeskyTaucs.hpp"
#include "ClpMessage.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
ClpCholeskyTaucs::ClpCholeskyTaucs () 
  : ClpCholeskyBase(),
    matrix_(NULL),
    factorization_(NULL),
    sparseFactor_(NULL),
    choleskyStart_(NULL),
    choleskyRow_(NULL),
    sizeFactor_(0),
    rowCopy_(NULL)
{
  type_=13;
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
ClpCholeskyTaucs::ClpCholeskyTaucs (const ClpCholeskyTaucs & rhs) 
: ClpCholeskyBase(rhs)
{
  type_=rhs.type_;
  // For Taucs stuff is done by malloc
  matrix_ = rhs.matrix_;
  sizeFactor_=rhs.sizeFactor_;
  if (matrix_) {
    choleskyStart_ = (int *) malloc((numberRows_+1)*sizeof(int));
    memcpy(choleskyStart_,rhs.choleskyStart_,(numberRows_+1)*sizeof(int));
    choleskyRow_ = (int *) malloc(sizeFactor_*sizeof(int));
    memcpy(choleskyRow_,rhs.choleskyRow_,sizeFactor_*sizeof(int));
    sparseFactor_ = (double *) malloc(sizeFactor_*sizeof(double));
    memcpy(sparseFactor_,rhs.sparseFactor_,sizeFactor_*sizeof(double));
    matrix_->colptr = choleskyStart_;
    matrix_->rowind = choleskyRow_;
    matrix_->values.d = sparseFactor_;
  } else {
    sparseFactor_=NULL;
    choleskyStart_=NULL;
    choleskyRow_=NULL;
  }
  factorization_=NULL,
  rowCopy_ = rhs.rowCopy_->clone();
}


//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
ClpCholeskyTaucs::~ClpCholeskyTaucs ()
{
  taucs_ccs_free(matrix_);
  if (factorization_)
    taucs_supernodal_factor_free(factorization_);
  delete rowCopy_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
ClpCholeskyTaucs &
ClpCholeskyTaucs::operator=(const ClpCholeskyTaucs& rhs)
{
  if (this != &rhs) {
    ClpCholeskyBase::operator=(rhs);
    taucs_ccs_free(matrix_);
    if (factorization_)
      taucs_supernodal_factor_free(factorization_);
    factorization_=NULL;
    sizeFactor_=rhs.sizeFactor_;
    matrix_ = rhs.matrix_;
    if (matrix_) {
      choleskyStart_ = (int *) malloc((numberRows_+1)*sizeof(int));
      memcpy(choleskyStart_,rhs.choleskyStart_,(numberRows_+1)*sizeof(int));
      choleskyRow_ = (int *) malloc(sizeFactor_*sizeof(int));
      memcpy(choleskyRow_,rhs.choleskyRow_,sizeFactor_*sizeof(int));
      sparseFactor_ = (double *) malloc(sizeFactor_*sizeof(double));
      memcpy(sparseFactor_,rhs.sparseFactor_,sizeFactor_*sizeof(double));
      matrix_->colptr = choleskyStart_;
      matrix_->rowind = choleskyRow_;
      matrix_->values.d = sparseFactor_;
    } else {
      sparseFactor_=NULL;
      choleskyStart_=NULL;
      choleskyRow_=NULL;
    }
    delete rowCopy_;
    rowCopy_ = rhs.rowCopy_->clone();
  }
  return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpCholeskyBase * ClpCholeskyTaucs::clone() const
{
  return new ClpCholeskyTaucs(*this);
}
/* Orders rows and saves pointer to matrix.and model */
int 
ClpCholeskyTaucs::order(ClpInterior * model) 
{
  numberRows_ = model->numberRows();
  rowsDropped_ = new char [numberRows_];
  memset(rowsDropped_,0,numberRows_);
  numberRowsDropped_=0;
  model_=model;
  rowCopy_ = model->clpMatrix()->reverseOrderedCopy();
  const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
  const int * columnLength = model_->clpMatrix()->getVectorLengths();
  const int * row = model_->clpMatrix()->getIndices();
  const CoinBigIndex * rowStart = rowCopy_->getVectorStarts();
  const int * rowLength = rowCopy_->getVectorLengths();
  const int * column = rowCopy_->getIndices();
  // We need two arrays for counts
  int * which = new int [numberRows_];
  int * used = new int[numberRows_];
  CoinZeroN(used,numberRows_);
  int iRow;
  sizeFactor_=0;
  for (iRow=0;iRow<numberRows_;iRow++) {
    int number=1;
    // make sure diagonal exists
    which[0]=iRow;
    used[iRow]=1;
    if (!rowsDropped_[iRow]) {
      CoinBigIndex startRow=rowStart[iRow];
      CoinBigIndex endRow=rowStart[iRow]+rowLength[iRow];
      for (CoinBigIndex k=startRow;k<endRow;k++) {
	int iColumn=column[k];
	CoinBigIndex start=columnStart[iColumn];
	CoinBigIndex end=columnStart[iColumn]+columnLength[iColumn];
	for (CoinBigIndex j=start;j<end;j++) {
	  int jRow=row[j];
	  if (jRow>=iRow&&!rowsDropped_[jRow]) {
	    if (!used[jRow]) {
	      used[jRow]=1;
	      which[number++]=jRow;
	    }
	  }
	}
      }
      sizeFactor_ += number;
      int j;
      for (j=0;j<number;j++)
	used[which[j]]=0;
    }
  }
  delete [] which;
  // Now we have size - create arrays and fill in
  matrix_ = taucs_ccs_create(numberRows_,numberRows_,sizeFactor_,
			     TAUCS_DOUBLE|TAUCS_SYMMETRIC|TAUCS_LOWER); 
  if (!matrix_) 
    return 1;
  // Space for starts
  choleskyStart_ = matrix_->colptr;
  choleskyRow_ = matrix_->rowind;
  sparseFactor_ = matrix_->values.d;
  sizeFactor_=0;
  which = choleskyRow_;
  for (iRow=0;iRow<numberRows_;iRow++) {
    int number=1;
    // make sure diagonal exists
    which[0]=iRow;
    used[iRow]=1;
    choleskyStart_[iRow]=sizeFactor_;
    if (!rowsDropped_[iRow]) {
      CoinBigIndex startRow=rowStart[iRow];
      CoinBigIndex endRow=rowStart[iRow]+rowLength[iRow];
      for (CoinBigIndex k=startRow;k<endRow;k++) {
	int iColumn=column[k];
	CoinBigIndex start=columnStart[iColumn];
	CoinBigIndex end=columnStart[iColumn]+columnLength[iColumn];
	for (CoinBigIndex j=start;j<end;j++) {
	  int jRow=row[j];
	  if (jRow>=iRow&&!rowsDropped_[jRow]) {
	    if (!used[jRow]) {
	      used[jRow]=1;
	      which[number++]=jRow;
	    }
	  }
	}
      }
      sizeFactor_ += number;
      int j;
      for (j=0;j<number;j++)
	used[which[j]]=0;
      // Sort
      std::sort(which,which+number);
      // move which on
      which += number;
    }
  }
  choleskyStart_[numberRows_]=sizeFactor_;
  delete [] used;
  permuteIn_ = new int [numberRows_];
  permuteOut_ = new int[numberRows_];
  int * perm, *invp;
  // There seem to be bugs in ordering if model too small
  if (numberRows_>10)
    taucs_ccs_order(matrix_,&perm,&invp,(const char *) "genmmd");
  else
    taucs_ccs_order(matrix_,&perm,&invp,(const char *) "identity");
  memcpy(permuteIn_,perm,numberRows_*sizeof(int));
  free(perm);
  memcpy(permuteOut_,invp,numberRows_*sizeof(int));
  free(invp);
  // need to permute
  taucs_ccs_matrix * permuted = taucs_ccs_permute_symmetrically(matrix_,permuteIn_,permuteOut_);
  // symbolic
  factorization_ = taucs_ccs_factor_llt_symbolic(permuted);
  taucs_ccs_free(permuted);
  return factorization_ ? 0 :1;
}
/* Factorize - filling in rowsDropped and returning number dropped */
int 
ClpCholeskyTaucs::factorize(const double * diagonal, int * rowsDropped) 
{
  const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
  const int * columnLength = model_->clpMatrix()->getVectorLengths();
  const int * row = model_->clpMatrix()->getIndices();
  const double * element = model_->clpMatrix()->getElements();
  const CoinBigIndex * rowStart = rowCopy_->getVectorStarts();
  const int * rowLength = rowCopy_->getVectorLengths();
  const int * column = rowCopy_->getIndices();
  const double * elementByRow = rowCopy_->getElements();
  int numberColumns=model_->clpMatrix()->getNumCols();
  int iRow;
  double * work = new double[numberRows_];
  CoinZeroN(work,numberRows_);
  const double * diagonalSlack = diagonal + numberColumns;
  int newDropped=0;
  double largest;
  //perturbation
  double perturbation=model_->diagonalPerturbation()*model_->diagonalNorm();
  perturbation=perturbation*perturbation;
  if (perturbation>1.0) {
    //if (model_->model()->logLevel()&4) 
      std::cout <<"large perturbation "<<perturbation<<std::endl;
    perturbation=sqrt(perturbation);;
    perturbation=1.0;
  } 
  for (iRow=0;iRow<numberRows_;iRow++) {
    double * put = sparseFactor_+choleskyStart_[iRow];
    int * which = choleskyRow_+choleskyStart_[iRow];
    int number = choleskyStart_[iRow+1]-choleskyStart_[iRow];
    if (!rowLength[iRow])
      rowsDropped_[iRow]=1;
    if (!rowsDropped_[iRow]) {
      CoinBigIndex startRow=rowStart[iRow];
      CoinBigIndex endRow=rowStart[iRow]+rowLength[iRow];
      work[iRow] = diagonalSlack[iRow];
      for (CoinBigIndex k=startRow;k<endRow;k++) {
	int iColumn=column[k];
	CoinBigIndex start=columnStart[iColumn];
	CoinBigIndex end=columnStart[iColumn]+columnLength[iColumn];
	double multiplier = diagonal[iColumn]*elementByRow[k];
	for (CoinBigIndex j=start;j<end;j++) {
	  int jRow=row[j];
	  if (jRow>=iRow&&!rowsDropped_[jRow]) {
	    double value=element[j]*multiplier;
	    work[jRow] += value;
	  }
	}
      }
      int j;
      for (j=0;j<number;j++) {
	int jRow = which[j];
	put[j]=work[jRow];
	work[jRow]=0.0;
      }
    } else {
      // dropped
      int j;
      for (j=1;j<number;j++) {
	put[j]=0.0;
      }
      put[0]=1.0;
    }
  }
  //check sizes
  double largest2=maximumAbsElement(sparseFactor_,sizeFactor_);
  largest2*=1.0e-19;
  largest = min (largest2,1.0e-11);
  int numberDroppedBefore=0;
  for (iRow=0;iRow<numberRows_;iRow++) {
    int dropped=rowsDropped_[iRow];
    // Move to int array
    rowsDropped[iRow]=dropped;
    if (!dropped) {
      CoinBigIndex start = choleskyStart_[iRow];
      double diagonal = sparseFactor_[start];
      if (diagonal>largest2) {
	sparseFactor_[start]=diagonal+perturbation;
      } else {
	sparseFactor_[start]=diagonal+perturbation;
	rowsDropped[iRow]=2;
	numberDroppedBefore++;
      } 
    } 
  }
  taucs_supernodal_factor_free_numeric(factorization_);
  // need to permute
  taucs_ccs_matrix * permuted = taucs_ccs_permute_symmetrically(matrix_,permuteIn_,permuteOut_);
  int rCode=taucs_ccs_factor_llt_numeric(permuted,factorization_);
  taucs_ccs_free(permuted);
  if (rCode)
    printf("return code of %d from factor\n",rCode);
  delete [] work;
  choleskyCondition_=1.0;
  bool cleanCholesky;
  if (model_->numberIterations()<200) 
    cleanCholesky=true;
  else 
    cleanCholesky=false;
  /*
    How do I find out where 1.0e100's are in cholesky?
  */
  if (cleanCholesky) {
    //drop fresh makes some formADAT easier
    int oldDropped=numberRowsDropped_;
    if (newDropped||numberRowsDropped_) {
      std::cout <<"Rank "<<numberRows_-newDropped<<" ( "<<
          newDropped<<" dropped)";
      if (newDropped>oldDropped) 
        std::cout<<" ( "<<newDropped-oldDropped<<" dropped this time)";
      std::cout<<std::endl;
      newDropped=0;
      for (int i=0;i<numberRows_;i++) {
	char dropped = rowsDropped[i];
	rowsDropped_[i]=dropped;
        if (dropped==2) {
          //dropped this time
          rowsDropped[newDropped++]=i;
          rowsDropped_[i]=0;
        } 
      } 
      numberRowsDropped_=newDropped;
      newDropped=-(1+newDropped);
    } 
  } else {
    if (newDropped) {
      newDropped=0;
      for (int i=0;i<numberRows_;i++) {
	char dropped = rowsDropped[i];
	int oldDropped = rowsDropped_[i];;
	rowsDropped_[i]=dropped;
        if (dropped==2) {
	  assert (!oldDropped);
          //dropped this time
          rowsDropped[newDropped++]=i;
          rowsDropped_[i]=1;
        } 
      } 
    } 
    numberRowsDropped_+=newDropped;
    if (numberRowsDropped_) {
      std::cout <<"Rank "<<numberRows_-numberRowsDropped_<<" ( "<<
          numberRowsDropped_<<" dropped)";
      if (newDropped) {
        std::cout<<" ( "<<newDropped<<" dropped this time)";
      } 
      std::cout<<std::endl;
    } 
  } 
  status_=0;
  return newDropped;
}
/* Uses factorization to solve. */
void 
ClpCholeskyTaucs::solve (double * region) 
{
  double * in = new double[numberRows_];
  double * out = new double[numberRows_];
  taucs_vec_permute(numberRows_,TAUCS_DOUBLE,region,in,permuteIn_);
  int rCode=taucs_supernodal_solve_llt(factorization_,out,in);
  if (rCode)
    printf("return code of %d from solve\n",rCode);
  taucs_vec_permute(numberRows_,TAUCS_DOUBLE,out,region,permuteOut_);
  delete [] out;
  delete [] in;
}
#endif