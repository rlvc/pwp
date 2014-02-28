/**
 * @file   cvfacedetector.cpp
 * @author Liangfu Chen <liangfu.chen@cn.fix8.com>
 * @date   Mon Jul  8 15:55:57 2013
 * 
 * @brief  
 * 
 * 
 */
#include "cvstageddetecthaar.h"

#define CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL

CvMat * icvCreateHaarFeatureSetEx(int tsize);
// 14 stages: .95,.99
//0,1,8,21,33,42,62,97,148,267,308,339,361,449,986
CvMat * get_haarclassifier_face20_v0();
// 10 stages, .85,.96
//0,1,2,10,29,73,103,160,196,201,316
CvMat * get_haarclassifier_face20_v1();

void icvWeightedThresholdClassify_v0(CvMat ** evalarr,
                                     double & thres, double & p)
{
  assert(CV_MAT_TYPE(evalarr[0]->type)==CV_32F);
  assert(CV_MAT_TYPE(evalarr[1]->type)==CV_32F);
  int i;
  double mu[2],minval,maxval;
  for (i=0;i<2;i++){mu[i]=cvAvg(evalarr[i]).val[0];}
  cvMinMaxLoc(evalarr[1],&minval,&maxval);
  if (mu[1]<mu[0]) {thres=maxval;p=1;} else {thres=minval;p=-1;}
}

void icvWeightedThresholdClassify_v1(CvMat ** evalarr,
                                     double & thres, double & p)
{
  assert(CV_MAT_TYPE(evalarr[0]->type)==CV_32F);
  assert(CV_MAT_TYPE(evalarr[1]->type)==CV_32F);
  int i,j,cols[2]={evalarr[0]->cols,evalarr[1]->cols};
  double ratio,sum,var[2],mu[2],minval=0xffffff,maxval=-0xfffff;
  float * evalptr[2];
  evalptr[0] = evalarr[0]->data.fl;
  evalptr[1] = evalarr[1]->data.fl;
  for (i=0;i<2;i++){
    sum=0;
    for (j=0;j<cols[i];j++){sum+=evalptr[i][j];}
    mu[i]=sum/double(cols[i]);
    sum=0;
    for (j=0;j<cols[i];j++){sum+=pow(evalptr[i][j]-mu[i],2.);}
    var[i]=sqrt(sum)/double(cols[i]);
  }
  for (j=0;j<cols[1];j++){
    minval=MIN(evalptr[1][j],minval);
    maxval=MAX(evalptr[1][j],maxval);
  }
  if (mu[1]<mu[0]) {
    ratio=var[1]/(var[0]+var[1]);//ratio=pow(ratio,4);
    thres=MIN(maxval,mu[1]+(mu[0]-mu[1])*ratio);p=1;
  } else {
    ratio=var[0]/(var[0]+var[1]);//ratio=pow(ratio,4);
    thres=MAX(minval,mu[1]-(mu[1]-mu[0])*ratio);p=-1;
  }
}

void icvWeightedThresholdClassify_v2(CvMat ** evalarr,
                                     double & thres, double & polar)
{
  assert(CV_MAT_TYPE(evalarr[0]->type)==CV_32F);
  assert(CV_MAT_TYPE(evalarr[1]->type)==CV_32F);
  double mu[2],minval,maxval;
  int i,j;float fi=1,di=1,ratio=.98;int fcc=0,dcc=0;
  float * evalresptr;
  for (i=0;i<2;i++){mu[i]=cvAvg(evalarr[i]).val[0];}
  cvMinMaxLoc(evalarr[1],&minval,&maxval);
  if (mu[1]<mu[0]) {
    thres=maxval;polar=1;
    while (di>ratio){
fcc=0;dcc=0;
thres-=1;
for (i=0;i<2;i++){
evalresptr=evalarr[i]->data.fl;
for (j=0;j<evalarr[i]->cols;j++){
if ((evalresptr[j]*polar<thres*polar)&&(i==0)) {fcc++;}
if ((evalresptr[j]*polar<thres*polar)&&(i==1)) {dcc++;}
}
}
fi=double(fcc)/double(evalarr[0]->cols);
di=double(dcc)/double(evalarr[1]->cols);
    }
  } else {
    thres=minval;polar=-1;
    while (di>ratio){
fcc=0;dcc=0;
thres+=1;
for (i=0;i<2;i++){
evalresptr=evalarr[i]->data.fl;
for (j=0;j<evalarr[i]->cols;j++){
if ((evalresptr[j]*polar<thres*polar)&&(i==0)) {fcc++;}
if ((evalresptr[j]*polar<thres*polar)&&(i==1)) {dcc++;}
}
}
fi=double(fcc)/double(evalarr[0]->cols);
di=double(dcc)/double(evalarr[1]->cols);
    }
  }
}

CV_INLINE
double icvEval(CvMat * imgIntegral, float * ftsptr)
{
  int * integral_data=imgIntegral->data.i;
  int i,step = imgIntegral->step/sizeof(int);
  int xx,yy,ww,hh,wt[3],p0[3],p1[3],p2[3],p3[3];
  double fval;
  for (i=0;i<3;i++){
    xx=cvRound((ftsptr+5*i)[0]);
    yy=cvRound((ftsptr+5*i)[1]);
    ww=cvRound((ftsptr+5*i)[2]);
    hh=cvRound((ftsptr+5*i)[3]);
    wt[i]=cvRound((ftsptr+5*i)[4]);
    if (i==2){if(wt[2]==0){break;}}
    p0[i]=xx+yy*step;
    p1[i]=xx+ww+yy*step;
    p2[i]=xx+(yy+hh)*step;
    p3[i]=xx+ww+(yy+hh)*step;
  }
  if (wt[2]==0){
    fval =
        (integral_data[p0[0]]-integral_data[p1[0]]-
         integral_data[p2[0]]+integral_data[p3[0]])*wt[0]+
        (integral_data[p0[1]]-integral_data[p1[1]]-
         integral_data[p2[1]]+integral_data[p3[1]])*wt[1];
  }else{
    fval =
        (integral_data[p0[0]]-integral_data[p1[0]]-
         integral_data[p2[0]]+integral_data[p3[0]])*wt[0]+
        (integral_data[p0[1]]-integral_data[p1[1]]-
         integral_data[p2[1]]+integral_data[p3[1]])*wt[1]+
        (integral_data[p0[2]]-integral_data[p1[2]]-
         integral_data[p2[2]]+integral_data[p3[2]])*wt[2];
  }
  return fval;
}

CV_INLINE
int icvIsTarget(CvMat * imgIntegral, CvRect roi, float * h, const int tsize)
{
  double scale = float(roi.width)/float(tsize);
  int i,step = imgIntegral->step/sizeof(int);
  int xx,yy,ww,hh,wt[3],p0[3],p1[3],p2[3],p3[3];
  double threshold=h[0];
  double polarity =h[1];
  for (i=0;i<3;i++){
    xx=cvRound((h+3+5*i)[0]*scale);
    yy=cvRound((h+3+5*i)[1]*scale);
    ww=cvRound((h+3+5*i)[2]*scale);
    hh=cvRound((h+3+5*i)[3]*scale);
    wt[i]=cvRound((h+3+5*i)[4]);
    if (i==2){if(wt[2]==0){break;}}
    p0[i]=xx+roi.x+(yy+roi.y)*step;
    p1[i]=xx+ww+roi.x+(yy+roi.y)*step;
    p2[i]=xx+roi.x+(yy+hh+roi.y)*step;
    p3[i]=xx+ww+roi.x+(yy+hh+roi.y)*step;
  }
  int * integral_data=imgIntegral->data.i;
  double fval;
  if (wt[2]==0){
    fval =
        (integral_data[p0[0]]-integral_data[p1[0]]-
         integral_data[p2[0]]+integral_data[p3[0]])*wt[0]+
        (integral_data[p0[1]]-integral_data[p1[1]]-
         integral_data[p2[1]]+integral_data[p3[1]])*wt[1];
  }else{
    fval =
        (integral_data[p0[0]]-integral_data[p1[0]]-
         integral_data[p2[0]]+integral_data[p3[0]])*wt[0]+
        (integral_data[p0[1]]-integral_data[p1[1]]-
         integral_data[p2[1]]+integral_data[p3[1]])*wt[1]+
        (integral_data[p0[2]]-integral_data[p1[2]]-
         integral_data[p2[2]]+integral_data[p3[2]])*wt[2];
  }

  return ((fval*polarity)<(threshold*pow(scale,2)*polarity))?1:0;
}

int CvStagedDetectorHaar::detect(CvMat * img, CvRect roi[])
{
  // assert(m_classifier);
  // assert(m_features);
  int nr=img->rows,nc=img->cols;
  CvMat * imgIntegral=cvCreateMat(nr+1,nc+1,CV_32S);
  cvIntegral(img,imgIntegral);
  int i,j,k,cc=0,maxcc=8000;

#if 0
  CvMat * haarclassifier = get_haarclassifier_face20_v0();
  int winsz,tsize=20;
  int num_classifiers=haarclassifier->rows;
  int nstages=14,stageiter;
  int stages_data[]={0,1,8,21,33,42,62,97,148,267,308,339,361,449,986,
                     num_classifiers};
  double thres00=.95,thres11=.99;
#else
  CvMat * haarclassifier = get_haarclassifier_face20_v1();
  int winsz,tsize=20;
  int num_classifiers=haarclassifier->rows;
  int nstages=10,stageiter;
  int stages_data[]={0,1,2,10,29,73,103,160,196,201,316,num_classifiers};
  double thres00=.85,thres11=.96;
#endif
  CvRect currroi;
  float h[18];double sum0,sum1,sum00,sum11;
  for (winsz=tsize;winsz<float(MIN(nr,nc))*.6;
       winsz=cvRound(float(winsz)*1.25)){
  for (i=0;i<nr-winsz;i=cvRound(float(i)+float(winsz)/10.)){
  for (j=0;j<nc-winsz;j=cvRound(float(j)+float(winsz)/10.)){

  // staged rejection
  currroi=cvRect(j,i,winsz,winsz);
  sum00=0;sum11=0;
  for (stageiter=0;stageiter<nstages;stageiter++){  
  sum0=0;sum1=0;
  for (k=stages_data[stageiter];
       k<MIN(num_classifiers,stages_data[stageiter+1]);k++)
  {
    // extract classifier
    memcpy(h,haarclassifier->data.ptr+sizeof(h)*k,sizeof(h));
    // examin the target
    sum0+=h[2]*float(icvIsTarget(imgIntegral,currroi,h,tsize));
    sum1+=h[2];
  } // k loop
  sum00+=sum0;sum11+=sum1;
  // if ((sum0<.99*sum1)||(k>=num_classifiers)){break;}
  if ((sum0<thres00*sum1)||(k>=num_classifiers)){break;}
  } // stageiter loop
  if ((sum00>=thres11*sum11)&&(k==num_classifiers)){roi[cc++]=currroi;}
  
  if(cc>=maxcc){break;}
  }
  if(cc>=maxcc){break;}
  }
  }
  
  cvReleaseMat(&imgIntegral);
  
  return cc;
}

int CvStagedDetectorHaar::
train(CvMat ** posimgs, int npos, CvMat ** negimgs, int nneg,
      int maxiter, int startiter)
{
  typedef void (*CvClassifierFuncType)(CvMat **, double &, double &);
  static CvClassifierFuncType icvWeightedThresholdClassify[3]={
    icvWeightedThresholdClassify_v0,
    icvWeightedThresholdClassify_v1,
    icvWeightedThresholdClassify_v2
  };
  
  int i,j,k,iter;
  int tsize = posimgs[0]->cols+1;
  if (tsize!=(posimgs[0]->rows+1)){return -1;}
  CvMat ** imgIntegral[2];
  CvMat * evalres[2];for(i=0;i<2;i++){evalres[i]=0;}
  int m=nneg,l=npos;
  // int m=2000,l=1000;
  int count[2]={m,l};
  if (!features) {
    features = icvCreateHaarFeatureSetEx(tsize);
    fprintf(stderr,"numfeatures: %d\n",features->rows);
  }
// cvPrintf(stderr,"%.0f,",features,cvRect(0,0,features->cols,5));
  int nfeatures = features->rows,nsamples=m+l;
  if (!weights) {
    weights = cvCreateMat((startiter==0)?8000:maxiter+1,nsamples,CV_64F);
  }
  double wtsum,invwtsum; double * wtptr; double epsval;
  for (i=0;i<2;i++){ imgIntegral[i]=new CvMat *[count[i]]; }
#ifdef CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL
  for (i=0;i<2;i++){
    if (!evalres_precomp[i]){
      evalres_precomp[i]=cvCreateMat(nfeatures,count[i],CV_32F);
      fprintf(stderr,"INFO: precompute eval[%d] allocated!\n",i);
    }
  }
#endif // CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL
  for (i=0;i<2;i++){ evalres[i]=cvCreateMat(1,count[i],CV_32F); }

  // compute integral images
  for (i=0;i<2;i++){
  for (j=0;j<count[i];j++){
    imgIntegral[i][j]=cvCreateMat(tsize,tsize,CV_32S);
    cvIntegral(i==0?negimgs[j]:posimgs[j],imgIntegral[i][j]);
  }
  }

  // initialize weights
  if (!weights_initialized){
  for (i=0;i<  m;i++){weights->data.db[i]=.5/double(m);} // negative 
  for (i=m;i<l+m;i++){weights->data.db[i]=.5/double(l);} // positive
  weights_initialized=1;
  }
  
#ifdef CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL
  // precompute evaluation results
  if (!evalres_precomputed){
  for (i=0;i<nfeatures;i++){
    float * ftsptr = features->data.fl+i*features->cols;
    // extract feature values from integral images
    for (j=0;j<2;j++){
      for (k=0;k<count[j];k++){
        CV_MAT_ELEM(*evalres_precomp[j],float,i,k)=
            icvEval(imgIntegral[j][k],ftsptr+3);
      }
    }if((i%10000)==1){fprintf(stderr,"-");}
  }fprintf(stderr,"\n");
  fprintf(stderr,"INFO: precompute eval complete!\n");
  evalres_precomputed=1;
  }
#endif // CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL

  // find a single feature in each loop
  // for (iter=startiter;iter<maxiter;iter++)
  iter=startiter;
  {
    CvMat * epsilon = cvCreateMat(1,nfeatures,CV_64F);
    cvSet(epsilon,cvScalar(-1));
    
    // normalize weights
    {
    wtsum=0;
    wtptr = weights->data.db+nsamples*iter;
    for (i=0;i<nsamples;i++){wtsum+=wtptr[i];}
    invwtsum=1./wtsum;
    for (i=0;i<nsamples;i++){wtptr[i]=wtptr[i]*invwtsum;}
    }

    for (i=iter;i<nfeatures;i++)
    {
      float * ftsptr = features->data.fl+i*features->cols;
      // extract feature values from integral images
      for (j=0;j<2;j++){
#ifdef CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL
        memcpy(evalres[j]->data.ptr,
               evalres_precomp[j]->data.ptr+i*evalres_precomp[j]->step,
               sizeof(float)*count[j]);
#else
      for (k=0;k<count[j];k++){
        evalres[j]->data.fl[k]=icvEval(imgIntegral[j][k],ftsptr+3);
      }
#endif // CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL
      }
      // approximate classifier parameters
      double threshold,polarity;
      icvWeightedThresholdClassify[1](evalres,threshold,polarity);
      ftsptr[0]=threshold;ftsptr[1]=polarity;

      // compute classification error
      epsval=0;
      for (j=0;j<2;j++){
      float * evalresptr = evalres[j]->data.fl;
      double * wt0ptr = wtptr+count[0]*j;
      for (k=0;k<count[j];k++){
        epsval+=wt0ptr[k]*
            fabs((((evalresptr[k]*polarity)<(threshold*polarity))?1:0)-j);
      }
      }
      epsilon->data.db[i]=epsval;
    } // end of i loop
    
    // min error feature classifier
    double minval=0xffffff; int minloc;
    for (i=iter;i<nfeatures;i++){
      double epsval = epsilon->data.db[i];
      if (epsval<minval){minval=epsval;minloc=i;}
    }
    swap(features->data.ptr+features->step*iter,
         features->data.ptr+features->step*minloc,
         features->step);
#ifdef CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL
    for (j=0;j<2;j++){
    swap(evalres_precomp[j]->data.ptr+evalres_precomp[j]->step*iter,
         evalres_precomp[j]->data.ptr+evalres_precomp[j]->step*minloc,
         evalres_precomp[j]->step);
    }
#endif // CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL
    
    // update the weights
    {
    double * wt0ptr = weights->data.db+nsamples*iter;
    double * wt1ptr = weights->data.db+nsamples*(iter+1);
    float thres = (features->data.fl+iter*features->cols)[0];
    float polar = (features->data.fl+iter*features->cols)[1];
    double beta = minval/(1.-minval),ei;
    (features->data.fl+iter*features->cols)[2]=log(1./(beta+1e-6));
    k=0;
    for (i=0;i<2;i++){
    float * evalresptr = evalres[i]->data.fl;
    for (j=0;j<count[i];j++,k++){
      ei = ((evalresptr[j]*polar)<(thres*polar))?1:0;
      wt1ptr[k]=wt0ptr[k]*pow(beta,ei);
    }
    }
    }
    
// fprintf(stderr,"minloc: %d(%f) at %dth iter!\n",minloc,minval,iter);
// fprintf(stderr," /* %04d */ ",iter);
// fprintf(stderr,"%f,",(features->data.fl+iter*features->cols)[0]);
// fprintf(stderr,"%.0f,",(features->data.fl+iter*features->cols)[1]);
// fprintf(stderr,"%f,",(features->data.fl+iter*features->cols)[2]);
// cvPrintf(stderr,"%.0f,",features,cvRect(3,iter,features->cols-3,1));

#if 0
    // display min err feature
    {
      CvMat * disp = cvCreateMat(tsize,tsize,CV_8U);
      cvSet(disp,cvScalar(128));
      // memcpy(disp->data.ptr,posimgs[0]->data.ptr,(tsize-1)*(tsize-1));
      float * curptr = features->data.fl+iter*features->cols+3;
      CvMat subdisp0_stub,subdisp1_stub;
      CvMat * subdisp0 = cvGetSubRect(disp,&subdisp0_stub,
                         cvRect(curptr[0],curptr[1],curptr[2],curptr[3]));
      cvSet(subdisp0,cvScalar(curptr[4]>0?255:0));
      CvMat * subdisp1 = cvGetSubRect(disp,&subdisp1_stub,
                         cvRect(curptr[5],curptr[6],curptr[7],curptr[8]));
      cvSet(subdisp1,cvScalar(curptr[9]>0?255:0));
      CV_SHOW(disp);
      cvReleaseMat(&disp);
    }
#endif

    cvReleaseMat(&epsilon);
// break;
  } // end of iter loop

  // release memory
  for (i=0;i<2;i++){cvReleaseMat(&evalres[i]);}
  // cvReleaseMat(&weights);
  // cvReleaseMat(&features);
  for (i=0;i<2;i++){
    for (j=0;j<count[i];j++){ cvReleaseMat(&imgIntegral[i][j]); }
    delete [] imgIntegral[i];
  }

  return 1;
}

// validate i-th classifier on the validation set
int CvStagedDetectorHaar::validate(int ni, double & fi, double & di)
{
#ifdef CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL
  int i,j,iter=ni-1;float thres,polar; int fcc=0,dcc=0;
  int count[2];
  for (i=0;i<2;i++) { count[i] = evalres_precomp[i]->cols; }
  CvMat * frate = cvCreateMat(1,count[0],CV_32S);
  CvMat * drate = cvCreateMat(1,count[1],CV_32S);
  cvSet(frate,cvScalar(1));
  cvSet(drate,cvScalar(1));
  int * frateptr = frate->data.i;
  int * drateptr = drate->data.i;
  for (iter=0;iter<ni;iter++){
  thres=CV_MAT_ELEM(*features,float,iter,0);
  polar=CV_MAT_ELEM(*features,float,iter,1);
  for (i=0;i<2;i++){
  float * evalresptr=
      (float*)(evalres_precomp[i]->data.ptr+evalres_precomp[i]->step*iter);
  for (j=0;j<evalres_precomp[i]->cols;j++){
    if (evalresptr[j]*polar<thres*polar) {
    }else{
      if (i==0) {frateptr[j]=0;}  // false positive
      if (i==1) {drateptr[j]=0;}  // detection ratio
    }
  } // j loop
  } // i loop
  } // iter loop
  fi=cvSum(frate).val[0]/double(count[0]);
  di=cvSum(drate).val[0]/double(count[1]);
  cvReleaseMat(&frate);
  cvReleaseMat(&drate);
#else
#error "not implemented!"
#endif // CV_STAGED_DETECT_HAAR_PRECOMPUTE_EVAL
}

// adjust i-th threshold to target detection ratio
int CvStagedDetectorHaar::adjust(int ni, double dtar,
                                 double & fi, double & di)
{
  float * featptr = (float*)(features->data.ptr+features->step*(ni-1));
  int i,j,iter=ni-1;float thres,polar; int fcc=0,dcc=0;
  float * evalresptr;
  if (featptr[1]>0){
    while(di<dtar){
      featptr[0]+=1;
fcc=0;dcc=0;
thres=CV_MAT_ELEM(*features,float,iter,0);
polar=CV_MAT_ELEM(*features,float,iter,1);
for (i=0;i<2;i++){
evalresptr=evalres_precomp[i]->data.fl+evalres_precomp[i]->cols*iter;
for (j=0;j<evalres_precomp[i]->cols;j++){
if ((evalresptr[j]*polar<thres*polar)&&(i==0)) {fcc++;}
if ((evalresptr[j]*polar<thres*polar)&&(i==1)) {dcc++;}
}
}
fi=double(fcc)/double(evalres_precomp[0]->cols);
di=double(dcc)/double(evalres_precomp[1]->cols);
    }
  }else{
    while(di<dtar){
      featptr[0]-=1;
fcc=0;dcc=0;
thres=CV_MAT_ELEM(*features,float,iter,0);
polar=CV_MAT_ELEM(*features,float,iter,1);
for (i=0;i<2;i++){
evalresptr=evalres_precomp[i]->data.fl+evalres_precomp[i]->cols*iter;
for (j=0;j<evalres_precomp[i]->cols;j++){
if ((evalresptr[j]*polar<thres*polar)&&(i==0)) {fcc++;}
if ((evalresptr[j]*polar<thres*polar)&&(i==1)) {dcc++;}
}
}
fi=double(fcc)/double(evalres_precomp[0]->cols);
di=double(dcc)/double(evalres_precomp[1]->cols);
    }
  }
  // fprintf(stderr,"ni:%d,thres:%f,di:%f\n",ni,featptr[0],di);
  return 1;
}

// cascade detector training framework 
int CvStagedDetectorHaar::
cascadetrain(CvMat ** posimgs, int npos, CvMat ** negimgs, int nneg,
             double fper, double dper, double ftarget)
{
  int i,ii,j,k,ni,maxiter=200; double fi,di;
  CvMat * Frate = cvCreateMat(1,maxiter,CV_64F); Frate->data.db[0]=1.0;
  CvMat * Drate = cvCreateMat(1,maxiter,CV_64F); Drate->data.db[0]=1.0;
  CvMat * stage = cvCreateMat(1,maxiter,CV_32S); stage->data.i[0]=0;

  for (i=0,ni=0;(Frate->data.db[i]>ftarget)&&(i<maxiter);)
  {
    i++;
    fi=1.0;
    for (;fi>fper*Frate->data.db[i-1];){
      // fprintf(stderr,"%f>%f\n",fi,fper*Frate->data.db[i-1]);
      ni++;

      // adaboost training
      train(posimgs,npos,negimgs,nneg,ni,ni-1);

      // validation
      validate(ni,fi,di);

      // adjust threshold for i-th classifier
      // adjust(ni,dper*Drate->data.db[i-1],fi,di);
      adjust(ni,dper,fi,di);

// update weights
// if (0)      
// {
//   int nfeatures=features->rows;
//   int count[2]={evalres_precomp[0]->cols,evalres_precomp[1]->cols};
//   int nsamples = count[0]+count[1];
//   CvMat * epsilon = cvCreateMat(1,nfeatures,CV_64F);
//   cvSet(epsilon,cvScalar(-1));
//   int iter=ni-1;double epsval;
//   float thres = CV_MAT_ELEM(*features,float,iter,0);
//   float polar = CV_MAT_ELEM(*features,float,iter,1);
//   double * wtptr;
//   for (ii=iter;ii<nfeatures;ii++){
//     wtptr = weights->data.db+nsamples*iter;
//     epsval=0;
//     for (j=0;j<2;j++){
//       float * evalresptr =
//           evalres_precomp[j]->data.fl+evalres_precomp[j]->cols*ii;
//       double * wt0ptr = wtptr+count[0]*j;
//       for (k=0;k<count[j];k++){
//         epsval+=wt0ptr[k]*
//             fabs((((evalresptr[k]*polar)<(thres*polar))?1:0)-j);
//       }
//     }
//     epsilon->data.db[ii]=epsval;
//   }
//   double minval=0xffffff; int minloc;
//   for (ii=iter;ii<nfeatures;ii++){
//     epsval = epsilon->data.db[ii];
//     if (epsval<minval){minval=epsval;minloc=ii;}
//   }
//   double * wt0ptr = weights->data.db+nsamples*iter;
//   double * wt1ptr = weights->data.db+nsamples*(iter+1);
//   // float thres = (features->data.fl+iter*features->cols)[0];
//   // float polar = (features->data.fl+iter*features->cols)[1];
//   double beta = minval/(1.-minval),ei;
//   (features->data.fl+iter*features->cols)[2]=log(1./(beta+1e-6));
//   k=0;
//   for (ii=0;ii<2;ii++){
//   // float * evalresptr = evalres[i]->data.fl;
//   float * evalresptr =
//       evalres_precomp[ii]->data.fl+evalres_precomp[ii]->cols*iter;
//   for (j=0;j<count[ii];j++,k++){
//     ei = ((evalresptr[j]*polar)<(thres*polar))?1:0;
//     wt1ptr[k]=wt0ptr[k]*pow(beta,ei);
//   }
//   }
//   cvReleaseMat(&epsilon);
// }

      // compute fi under this threshold
      validate(ni,fi,di);
      //if (fi==fi_old){ni--;}
fprintf(stderr,"/* %d,fi:%.4f,di:%.2f */",ni-1,fi,di);
fprintf(stderr,"%.2f,",(features->data.fl+(ni-1)*features->cols)[0]);
fprintf(stderr,"%.0f,",(features->data.fl+(ni-1)*features->cols)[1]);
fprintf(stderr,"%.2f,",(features->data.fl+(ni-1)*features->cols)[2]);
cvPrintf(stderr,"%.0f,",features,cvRect(3,ni-1,features->cols-3,1));
    }
    Frate->data.db[i]=fi;//Frate->data.db[i-1]*fi;
    Drate->data.db[i]=di;//Drate->data.db[i-1];
    stage->data.i[i]=ni;

    // add to negative training set ...
    fprintf(stderr,"// end of %d-th stage with %d classifiers\n",i,ni);
  }

  {
    fprintf(stderr,"/* %d stages: */\n//",i);
    for (ii=0;ii<i+1;ii++){fprintf(stderr,"%d,",stage->data.i[ii]);}
  }
  
  cvReleaseMat(&Frate);
  cvReleaseMat(&Drate);
  cvReleaseMat(&stage);
  
  return 1;
}

CvMat * icvCreateHaarFeatureSetEx(int tsize)
{
  typedef float haar[18]; assert(sizeof(haar)==18*4);
  int log2count=8;
  haar * buff = (haar*)malloc(sizeof(haar)*(1<<log2count));
  int i,j,m,n,count=0,pstep=2,x,y,dx,dy;

  // for (x=0;x<tsize;x+=pstep){
  // for (y=0;y<tsize;y+=pstep){
  for (x=0;x<tsize;x++){
  for (y=0;y<tsize;y++){
  for (dx=2;dx<tsize;dx+=pstep){
  for (dy=2;dy<tsize;dy+=pstep){

  if ((1<<log2count)-10<count){
    buff = (haar*)realloc(buff,sizeof(haar)*(1<<(++log2count)));
    if (!buff) {
      fprintf(stderr, "ERROR: memory allocation failure!\n"); exit(1);
    }
  }

  if ((x+dx*2<tsize)&&(y+dy<tsize)){
    float tmp[18]={0,0,0,x,y,dx*2,dy,-1,x+dx,y,dx,dy,2,0,0,0,0,0};
    memcpy(buff[count++],tmp,sizeof(haar));
  }
  if ((x+dx*2<tsize)&&(y+dy<tsize)){
    float tmp[18]={0,0,0,y,x,dy,dx*2,-1,y,x+dx,dy,dx,2,0,0,0,0,0};
    memcpy(buff[count++],tmp,sizeof(haar));
  }
  if ((x+dx*3<tsize)&&(y+dy<tsize)){
    float tmp[18]={0,0,0,x,y,dx*3,dy,-1,x+dx,y,dx,dy,3,0,0,0,0,0};
    memcpy(buff[count++],tmp,sizeof(haar));
  }
  if ((x+dx*3<tsize)&&(y+dy<tsize)){
    float tmp[18]={0,0,0,y,x,dy,dx*3,-1,y,x+dx,dy,dx,3,0,0,0,0,0};
    memcpy(buff[count++],tmp,sizeof(haar));
  }
  if ((x+dx*4<tsize)&&(y+dy<tsize)){
    float tmp[18]={0,0,0,x,y,dx*4,dy,-1,x+dx,y,dx*2,dy,2,0,0,0,0,0};
    memcpy(buff[count++],tmp,sizeof(haar));
  }
  if ((x+dx*4<tsize)&&(y+dy<tsize)){
    float tmp[18]={0,0,0,y,x,dy,dx*4,-1,y,x+dx,dy,dx*2,2,0,0,0,0,0};
    memcpy(buff[count++],tmp,sizeof(haar));
  }
  if ((x+dx*2<tsize)&&(y+dy*2<tsize)){
    float tmp[18]={0,0,0,x,y,dx*2,dy*2,-1,x,y,dx,dy,2,x+dx,y+dy,dx,dy,2};
    memcpy(buff[count++],tmp,sizeof(haar));
  }
  
  }
  }
  }
  }

  CvMat * features = cvCreateMat(count,18,CV_32F);
  memcpy(features->data.ptr,buff,sizeof(haar)*count);
  free(buff);
  return features;
}

// 14 stages: .95,.99
//0,1,8,21,33,42,62,97,148,267,308,339,361,449,986
CvMat * get_haarclassifier_face20_v0()
{
  static float haarclassifier_face20_data[]={
/* 0,fi:0.1759,di:0.94 */423.17,-1,2.02,7,2,6,8,-1,9,2,2,8,3,0,0,0,0,0,
// end of 1-th stage with 1 classifiers
/* 1,fi:0.1625,di:0.93 */375.63,-1,1.98,7,3,6,8,-1,9,3,2,8,3,0,0,0,0,0,
/* 2,fi:0.1568,di:0.93 */452.57,-1,1.97,7,2,6,10,-1,9,2,2,10,3,0,0,0,0,0,
/* 3,fi:0.1515,di:0.93 */502.20,-1,1.96,7,1,6,10,-1,9,1,2,10,3,0,0,0,0,0,
/* 4,fi:0.1467,di:0.92 */484.34,-1,1.94,7,0,6,10,-1,9,0,2,10,3,0,0,0,0,0,
/* 5,fi:0.1427,di:0.92 */259.50,-1,1.93,7,4,6,6,-1,9,4,2,6,3,0,0,0,0,0,
/* 6,fi:0.1414,di:0.92 */396.37,-1,1.93,7,1,6,8,-1,9,1,2,8,3,0,0,0,0,0,
/* 7,fi:0.0488,di:0.88 */413.52,-1,1.91,1,4,18,4,-1,1,6,18,2,2,0,0,0,0,0,
// end of 2-th stage with 8 classifiers
/* 8,fi:0.0479,di:0.87 */284.42,-1,1.91,7,3,6,6,-1,9,3,2,6,3,0,0,0,0,0,
/* 9,fi:0.0464,di:0.87 */535.69,-1,1.91,7,0,6,12,-1,9,0,2,12,3,0,0,0,0,0,
/* 10,fi:0.0431,di:0.86 */361.35,-1,1.90,3,4,16,4,-1,3,6,16,2,2,0,0,0,0,0,
/* 11,fi:0.0409,di:0.86 */396.64,-1,1.89,0,4,18,4,-1,0,6,18,2,2,0,0,0,0,0,
/* 12,fi:0.0405,di:0.86 */352.92,-1,1.89,2,4,16,4,-1,2,6,16,2,2,0,0,0,0,0,
/* 13,fi:0.0402,di:0.86 */357.49,-1,1.88,7,3,6,10,-1,9,3,2,10,3,0,0,0,0,0,
/* 14,fi:0.0380,di:0.85 */444.93,-1,1.88,7,2,6,14,-1,9,2,2,14,3,0,0,0,0,0,
/* 15,fi:0.0380,di:0.85 */491.06,-1,1.88,7,1,6,12,-1,9,1,2,12,3,0,0,0,0,0,
/* 16,fi:0.0378,di:0.85 */285.72,-1,1.88,7,4,6,8,-1,9,4,2,8,3,0,0,0,0,0,
/* 17,fi:0.0367,di:0.84 */272.03,-1,1.87,7,2,6,6,-1,9,2,2,6,3,0,0,0,0,0,
/* 18,fi:0.0363,di:0.84 */353.80,-1,1.87,1,4,16,4,-1,1,6,16,2,2,0,0,0,0,0,
/* 19,fi:0.0361,di:0.84 */499.60,-1,1.87,7,1,6,16,-1,9,1,2,16,3,0,0,0,0,0,
/* 20,fi:0.0262,di:0.82 */1647.05,-1,1.87,1,3,18,12,-1,1,7,18,4,3,0,0,0,0,0,
// end of 3-th stage with 21 classifiers
/* 21,fi:0.0262,di:0.82 */423.23,-1,1.86,7,2,6,12,-1,9,2,2,12,3,0,0,0,0,0,
/* 22,fi:0.0222,di:0.80 */481.43,-1,1.86,3,4,16,8,-1,3,6,16,4,2,0,0,0,0,0,
/* 23,fi:0.0216,di:0.80 */315.18,-1,1.86,4,4,14,4,-1,4,6,14,2,2,0,0,0,0,0,
/* 24,fi:0.0216,di:0.79 */399.28,-1,1.86,4,4,14,8,-1,4,6,14,4,2,0,0,0,0,0,
/* 25,fi:0.0213,di:0.79 */390.20,-1,1.86,3,4,14,8,-1,3,6,14,4,2,0,0,0,0,0,
/* 26,fi:0.0211,di:0.79 */303.78,-1,1.86,3,4,14,4,-1,3,6,14,2,2,0,0,0,0,0,
/* 27,fi:0.0207,di:0.79 */1573.57,-1,1.85,0,3,18,12,-1,0,7,18,4,3,0,0,0,0,0,
/* 28,fi:0.0200,di:0.79 */280.57,-1,1.85,2,4,14,4,-1,2,6,14,2,2,0,0,0,0,0,
/* 29,fi:0.0198,di:0.79 */445.96,-1,1.85,2,4,16,8,-1,2,6,16,4,2,0,0,0,0,0,
/* 30,fi:0.0189,di:0.79 */414.36,-1,1.85,7,3,6,14,-1,9,3,2,14,3,0,0,0,0,0,
/* 31,fi:0.0185,di:0.79 */323.35,-1,1.85,0,4,16,4,-1,0,6,16,2,2,0,0,0,0,0,
/* 32,fi:0.0176,di:0.78 */207.73,-1,1.85,7,5,6,6,-1,9,5,2,6,3,0,0,0,0,0,
// end of 4-th stage with 33 classifiers
/* 33,fi:0.0176,di:0.78 */520.08,-1,1.85,1,4,18,8,-1,1,6,18,4,2,0,0,0,0,0,
/* 34,fi:0.0176,di:0.78 */470.23,-1,1.84,7,2,6,16,-1,9,2,2,16,3,0,0,0,0,0,
/* 35,fi:0.0174,di:0.78 */1456.32,-1,1.84,2,3,16,12,-1,2,7,16,4,3,0,0,0,0,0,
/* 36,fi:0.0172,di:0.78 */432.20,-1,1.83,1,4,16,8,-1,1,6,16,4,2,0,0,0,0,0,
/* 37,fi:0.0169,di:0.78 */406.43,-1,1.83,5,4,14,8,-1,5,6,14,4,2,0,0,0,0,0,
/* 38,fi:0.0165,di:0.78 */1471.48,-1,1.83,3,3,16,12,-1,3,7,16,4,3,0,0,0,0,0,
/* 39,fi:0.0165,di:0.77 */452.79,-1,1.83,7,1,6,14,-1,9,1,2,14,3,0,0,0,0,0,
/* 40,fi:0.0163,di:0.77 */342.20,-1,1.83,7,3,6,12,-1,9,3,2,12,3,0,0,0,0,0,
/* 41,fi:0.0088,di:0.76 */266.30,-1,1.82,4,3,2,12,-1,4,7,2,4,3,0,0,0,0,0,
// end of 5-th stage with 42 classifiers
/* 42,fi:0.0088,di:0.76 */1382.75,-1,1.82,1,3,16,12,-1,1,7,16,4,3,0,0,0,0,0,
/* 43,fi:0.0088,di:0.76 */508.85,-1,1.82,0,4,18,8,-1,0,6,18,4,2,0,0,0,0,0,
/* 44,fi:0.0086,di:0.75 */1201.86,-1,1.82,1,2,18,12,-1,1,6,18,4,3,0,0,0,0,0,
/* 45,fi:0.0084,di:0.75 */273.06,-1,1.82,1,4,14,4,-1,1,6,14,2,2,0,0,0,0,0,
/* 46,fi:0.0081,di:0.75 */1185.29,-1,1.81,1,2,18,8,-1,1,6,18,4,2,0,0,0,0,0,
/* 47,fi:0.0081,di:0.75 */506.13,-1,1.81,7,0,6,16,-1,9,0,2,16,3,0,0,0,0,0,
/* 48,fi:0.0081,di:0.75 */1067.71,-1,1.81,3,2,16,12,-1,3,6,16,4,3,0,0,0,0,0,
/* 49,fi:0.0081,di:0.74 */348.52,-1,1.81,7,0,6,8,-1,9,0,2,8,3,0,0,0,0,0,
/* 50,fi:0.0079,di:0.74 */439.78,-1,1.80,0,4,16,8,-1,0,6,16,4,2,0,0,0,0,0,
/* 51,fi:0.0079,di:0.74 */217.36,-1,1.80,4,4,12,4,-1,4,6,12,2,2,0,0,0,0,0,
/* 52,fi:0.0077,di:0.74 */304.75,-1,1.80,5,4,12,8,-1,5,6,12,4,2,0,0,0,0,0,
/* 53,fi:0.0077,di:0.74 */494.95,-1,1.80,7,0,6,14,-1,9,0,2,14,3,0,0,0,0,0,
/* 54,fi:0.0062,di:0.74 */614.15,-1,1.80,14,3,4,12,-1,14,7,4,4,3,0,0,0,0,0,
/* 55,fi:0.0062,di:0.73 */1189.22,-1,1.80,0,2,18,8,-1,0,6,18,4,2,0,0,0,0,0,
/* 56,fi:0.0062,di:0.73 */373.22,-1,1.79,2,4,14,8,-1,2,6,14,4,2,0,0,0,0,0,
/* 57,fi:0.0062,di:0.73 */274.96,-1,1.79,7,1,6,6,-1,9,1,2,6,3,0,0,0,0,0,
/* 58,fi:0.0062,di:0.73 */303.09,-1,1.79,5,4,14,4,-1,5,6,14,2,2,0,0,0,0,0,
/* 59,fi:0.0062,di:0.73 */401.33,-1,1.79,7,3,6,16,-1,9,3,2,16,3,0,0,0,0,0,
/* 60,fi:0.0062,di:0.73 */313.88,-1,1.79,4,4,12,8,-1,4,6,12,4,2,0,0,0,0,0,
/* 61,fi:0.0057,di:0.72 */503.49,-1,1.79,2,3,4,12,-1,2,7,4,4,3,0,0,0,0,0,
// end of 6-th stage with 62 classifiers
/* 62,fi:0.0057,di:0.72 */1197.41,-1,1.79,3,3,14,12,-1,3,7,14,4,3,0,0,0,0,0,
/* 63,fi:0.0057,di:0.72 */198.94,-1,1.79,3,4,12,4,-1,3,6,12,2,2,0,0,0,0,0,
/* 64,fi:0.0057,di:0.72 */1111.52,-1,1.79,3,2,16,8,-1,3,6,16,4,2,0,0,0,0,0,
/* 65,fi:0.0057,di:0.71 */-548.59,1,1.78,2,0,16,8,-1,2,2,16,4,2,0,0,0,0,0,
/* 66,fi:0.0057,di:0.71 */1177.12,-1,1.78,0,2,18,12,-1,0,6,18,4,3,0,0,0,0,0,
/* 67,fi:0.0057,di:0.71 */497.31,-1,1.78,7,1,6,18,-1,9,1,2,18,3,0,0,0,0,0,
/* 68,fi:0.0057,di:0.71 */299.90,-1,1.78,7,4,6,12,-1,9,4,2,12,3,0,0,0,0,0,
/* 69,fi:0.0057,di:0.71 */518.88,-1,1.78,7,0,6,18,-1,9,0,2,18,3,0,0,0,0,0,
/* 70,fi:0.0057,di:0.71 */144.55,-1,1.78,7,4,6,4,-1,9,4,2,4,3,0,0,0,0,0,
/* 71,fi:0.0057,di:0.71 */144.27,-1,1.78,7,3,6,4,-1,9,3,2,4,3,0,0,0,0,0,
/* 72,fi:0.0057,di:0.71 */793.61,-1,1.78,13,3,6,12,-1,13,7,6,4,3,0,0,0,0,0,
/* 73,fi:0.0057,di:0.70 */529.45,-1,1.78,13,3,4,12,-1,13,7,4,4,3,0,0,0,0,0,
/* 74,fi:0.0057,di:0.70 */1046.32,-1,1.78,1,2,16,8,-1,1,6,16,4,2,0,0,0,0,0,
/* 75,fi:0.0057,di:0.70 */1045.62,-1,1.78,2,2,16,12,-1,2,6,16,4,3,0,0,0,0,0,
/* 76,fi:0.0057,di:0.70 */265.51,-1,1.78,7,4,6,10,-1,9,4,2,10,3,0,0,0,0,0,
/* 77,fi:0.0055,di:0.70 */285.54,-1,1.78,3,3,2,12,-1,3,7,2,4,3,0,0,0,0,0,
/* 78,fi:0.0053,di:0.70 */200.88,-1,1.78,2,4,12,4,-1,2,6,12,2,2,0,0,0,0,0,
/* 79,fi:0.0053,di:0.70 */903.92,-1,1.78,4,2,14,12,-1,4,6,14,4,3,0,0,0,0,0,
/* 80,fi:0.0053,di:0.70 */1200.15,-1,1.78,4,3,14,12,-1,4,7,14,4,3,0,0,0,0,0,
/* 81,fi:0.0053,di:0.70 */1070.24,-1,1.78,2,2,16,8,-1,2,6,16,4,2,0,0,0,0,0,
/* 82,fi:0.0053,di:0.70 */139.67,-1,1.77,7,5,6,4,-1,9,5,2,4,3,0,0,0,0,0,
/* 83,fi:0.0046,di:0.70 */307.15,-1,1.78,14,3,2,12,-1,14,7,2,4,3,0,0,0,0,0,
/* 84,fi:0.0046,di:0.69 */363.27,-1,1.78,1,4,14,8,-1,1,6,14,4,2,0,0,0,0,0,
/* 85,fi:0.0046,di:0.69 */-548.36,1,1.77,1,0,16,8,-1,1,2,16,4,2,0,0,0,0,0,
/* 86,fi:0.0046,di:0.69 */211.42,-1,1.77,5,4,12,4,-1,5,6,12,2,2,0,0,0,0,0,
/* 87,fi:0.0046,di:0.69 */1116.02,-1,1.77,2,3,14,12,-1,2,7,14,4,3,0,0,0,0,0,
/* 88,fi:0.0046,di:0.69 */313.97,-1,1.77,3,4,12,8,-1,3,6,12,4,2,0,0,0,0,0,
/* 89,fi:0.0046,di:0.69 */-584.21,1,1.76,1,0,18,8,-1,1,2,18,4,2,0,0,0,0,0,
/* 90,fi:0.0046,di:0.69 */141.66,-1,1.76,10,4,8,4,-1,10,6,8,2,2,0,0,0,0,0,
/* 91,fi:0.0046,di:0.69 */296.13,-1,1.76,6,4,12,8,-1,6,6,12,4,2,0,0,0,0,0,
/* 92,fi:0.0046,di:0.69 */1254.92,-1,1.76,0,3,16,12,-1,0,7,16,4,3,0,0,0,0,0,
/* 93,fi:0.0046,di:0.69 */-550.49,1,1.76,3,0,16,8,-1,3,2,16,4,2,0,0,0,0,0,
/* 94,fi:0.0046,di:0.68 */609.51,-1,1.76,1,3,6,12,-1,1,7,6,4,3,0,0,0,0,0,
/* 95,fi:0.0044,di:0.68 */693.50,-1,1.75,12,3,6,12,-1,12,7,6,4,3,0,0,0,0,0,
/* 96,fi:0.0040,di:0.68 */-506.62,1,1.75,4,0,14,8,-1,4,2,14,4,2,0,0,0,0,0,
// end of 7-th stage with 97 classifiers
/* 97,fi:0.0040,di:0.68 */317.03,-1,1.75,7,4,6,14,-1,9,4,2,14,3,0,0,0,0,0,
/* 98,fi:0.0040,di:0.68 */811.12,-1,1.75,11,3,8,12,-1,11,7,8,4,3,0,0,0,0,0,
/* 99,fi:0.0040,di:0.68 */990.95,-1,1.75,1,2,16,12,-1,1,6,16,4,3,0,0,0,0,0,
/* 100,fi:0.0040,di:0.68 */185.00,-1,1.75,9,4,10,4,-1,9,6,10,2,2,0,0,0,0,0,
/* 101,fi:0.0040,di:0.68 */922.05,-1,1.75,9,3,10,12,-1,9,7,10,4,3,0,0,0,0,0,
/* 102,fi:0.0040,di:0.68 */243.44,-1,1.75,0,4,14,4,-1,0,6,14,2,2,0,0,0,0,0,
/* 103,fi:0.0040,di:0.68 */219.03,-1,1.75,7,4,12,4,-1,7,6,12,2,2,0,0,0,0,0,
/* 104,fi:0.0040,di:0.67 */222.74,-1,1.75,9,4,10,8,-1,9,6,10,4,2,0,0,0,0,0,
/* 105,fi:0.0040,di:0.67 */236.86,-1,1.75,4,4,10,8,-1,4,6,10,4,2,0,0,0,0,0,
/* 106,fi:0.0040,di:0.67 */-551.77,1,1.75,0,0,18,8,-1,0,2,18,4,2,0,0,0,0,0,
/* 107,fi:0.0040,di:0.67 */299.52,-1,1.74,7,4,12,8,-1,7,6,12,4,2,0,0,0,0,0,
/* 108,fi:0.0040,di:0.67 */288.41,-1,1.74,1,4,12,8,-1,1,6,12,4,2,0,0,0,0,0,
/* 109,fi:0.0040,di:0.67 */206.91,-1,1.74,1,4,12,4,-1,1,6,12,2,2,0,0,0,0,0,
/* 110,fi:0.0040,di:0.66 */757.65,-1,1.74,10,3,8,12,-1,10,7,8,4,3,0,0,0,0,0,
/* 111,fi:0.0040,di:0.66 */-502.70,1,1.74,3,0,14,8,-1,3,2,14,4,2,0,0,0,0,0,
/* 112,fi:0.0040,di:0.66 */325.00,-1,1.74,15,3,2,12,-1,15,7,2,4,3,0,0,0,0,0,
/* 113,fi:0.0037,di:0.66 */106.46,-1,1.74,7,6,6,4,-1,9,6,2,4,3,0,0,0,0,0,
/* 114,fi:0.0037,di:0.66 */1137.69,-1,1.75,5,3,14,12,-1,5,7,14,4,3,0,0,0,0,0,
/* 115,fi:0.0037,di:0.66 */861.85,-1,1.74,3,2,14,12,-1,3,6,14,4,3,0,0,0,0,0,
/* 116,fi:0.0035,di:0.65 */398.69,-1,1.74,14,3,4,8,-1,14,7,4,4,2,0,0,0,0,0,
/* 117,fi:0.0035,di:0.65 */880.61,-1,1.74,5,2,14,12,-1,5,6,14,4,3,0,0,0,0,0,
/* 118,fi:0.0035,di:0.65 */663.61,-1,1.74,1,3,8,12,-1,1,7,8,4,3,0,0,0,0,0,
/* 119,fi:0.0035,di:0.65 */228.84,-1,1.74,8,4,10,8,-1,8,6,10,4,2,0,0,0,0,0,
/* 120,fi:0.0035,di:0.65 */174.75,-1,1.73,8,4,10,4,-1,8,6,10,2,2,0,0,0,0,0,
/* 121,fi:0.0035,di:0.65 */301.42,-1,1.73,2,4,12,8,-1,2,6,12,4,2,0,0,0,0,0,
/* 122,fi:0.0035,di:0.65 */442.80,-1,1.73,3,3,4,12,-1,3,7,4,4,3,0,0,0,0,0,
/* 123,fi:0.0035,di:0.65 */903.23,-1,1.73,4,2,14,8,-1,4,6,14,4,2,0,0,0,0,0,
/* 124,fi:0.0035,di:0.65 */-483.15,1,1.73,2,0,14,8,-1,2,2,14,4,2,0,0,0,0,0,
/* 125,fi:0.0035,di:0.65 */1014.44,-1,1.73,1,3,14,12,-1,1,7,14,4,3,0,0,0,0,0,
/* 126,fi:0.0035,di:0.65 */249.44,-1,1.73,3,4,10,8,-1,3,6,10,4,2,0,0,0,0,0,
/* 127,fi:0.0035,di:0.65 */975.29,-1,1.73,0,2,16,12,-1,0,6,16,4,3,0,0,0,0,0,
/* 128,fi:0.0035,di:0.65 */220.48,-1,1.72,6,4,12,4,-1,6,6,12,2,2,0,0,0,0,0,
/* 129,fi:0.0035,di:0.65 */242.69,-1,1.72,2,4,10,8,-1,2,6,10,4,2,0,0,0,0,0,
/* 130,fi:0.0035,di:0.64 */185.84,-1,1.72,7,5,6,8,-1,9,5,2,8,3,0,0,0,0,0,
/* 131,fi:0.0035,di:0.64 */357.10,-1,1.72,0,4,14,8,-1,0,6,14,4,2,0,0,0,0,0,
/* 132,fi:0.0035,di:0.64 */945.77,-1,1.72,0,2,16,8,-1,0,6,16,4,2,0,0,0,0,0,
/* 133,fi:0.0035,di:0.64 */-414.80,1,1.72,4,0,12,8,-1,4,2,12,4,2,0,0,0,0,0,
/* 134,fi:0.0035,di:0.64 */826.99,-1,1.72,2,2,14,12,-1,2,6,14,4,3,0,0,0,0,0,
/* 135,fi:0.0035,di:0.64 */538.71,-1,1.72,11,3,6,12,-1,11,7,6,4,3,0,0,0,0,0,
/* 136,fi:0.0035,di:0.64 */540.16,-1,1.72,2,3,6,12,-1,2,7,6,4,3,0,0,0,0,0,
/* 137,fi:0.0035,di:0.64 */911.59,-1,1.71,3,2,14,8,-1,3,6,14,4,2,0,0,0,0,0,
/* 138,fi:0.0033,di:0.64 */405.42,-1,1.71,1,3,4,12,-1,1,7,4,4,3,0,0,0,0,0,
/* 139,fi:0.0029,di:0.62 */-234.54,1,1.71,3,1,16,6,-1,3,3,16,2,3,0,0,0,0,0,
/* 140,fi:0.0029,di:0.62 */819.40,-1,1.71,4,3,12,12,-1,4,7,12,4,3,0,0,0,0,0,
/* 141,fi:0.0029,di:0.62 */828.89,-1,1.71,8,3,10,12,-1,8,7,10,4,3,0,0,0,0,0,
/* 142,fi:0.0029,di:0.62 */625.90,-1,1.71,9,3,8,12,-1,9,7,8,4,3,0,0,0,0,0,
/* 143,fi:0.0029,di:0.62 */-408.05,1,1.71,1,0,14,8,-1,1,2,14,4,2,0,0,0,0,0,
/* 144,fi:0.0029,di:0.62 */634.55,-1,1.71,2,3,8,12,-1,2,7,8,4,3,0,0,0,0,0,
/* 145,fi:0.0029,di:0.62 */761.11,-1,1.71,1,3,10,12,-1,1,7,10,4,3,0,0,0,0,0,
/* 146,fi:0.0029,di:0.62 */-454.16,1,1.71,0,0,16,8,-1,0,2,16,4,2,0,0,0,0,0,
/* 147,fi:0.0026,di:0.62 */805.16,-1,1.71,2,3,16,8,-1,2,7,16,4,2,0,0,0,0,0,
// end of 8-th stage with 148 classifiers
/* 148,fi:0.0026,di:0.62 */219.11,-1,1.70,5,4,10,8,-1,5,6,10,4,2,0,0,0,0,0,
/* 149,fi:0.0024,di:0.62 */538.00,-1,1.70,15,3,4,12,-1,15,7,4,4,3,0,0,0,0,0,
/* 150,fi:0.0024,di:0.61 */174.51,-1,1.70,3,3,2,8,-1,3,7,2,4,2,0,0,0,0,0,
/* 151,fi:0.0024,di:0.61 */217.80,-1,1.70,15,3,2,8,-1,15,7,2,4,2,0,0,0,0,0,
/* 152,fi:0.0024,di:0.61 */824.73,-1,1.70,5,3,12,12,-1,5,7,12,4,3,0,0,0,0,0,
/* 153,fi:0.0024,di:0.61 */297.53,-1,1.70,0,4,12,8,-1,0,6,12,4,2,0,0,0,0,0,
/* 154,fi:0.0024,di:0.61 */200.69,-1,1.70,4,4,8,8,-1,4,6,8,4,2,0,0,0,0,0,
/* 155,fi:0.0024,di:0.61 */915.54,-1,1.70,1,3,18,8,-1,1,7,18,4,2,0,0,0,0,0,
/* 156,fi:0.0024,di:0.61 */146.07,-1,1.70,11,4,8,4,-1,11,6,8,2,2,0,0,0,0,0,
/* 157,fi:0.0024,di:0.61 */657.69,-1,1.70,4,2,12,12,-1,4,6,12,4,3,0,0,0,0,0,
/* 158,fi:0.0024,di:0.60 */198.68,-1,1.69,14,3,2,8,-1,14,7,2,4,2,0,0,0,0,0,
/* 159,fi:0.0024,di:0.60 */-373.52,1,1.69,3,0,12,8,-1,3,2,12,4,2,0,0,0,0,0,
/* 160,fi:0.0024,di:0.60 */223.87,-1,1.69,3,4,8,8,-1,3,6,8,4,2,0,0,0,0,0,
/* 161,fi:0.0024,di:0.60 */867.72,-1,1.69,2,2,14,8,-1,2,6,14,4,2,0,0,0,0,0,
/* 162,fi:0.0024,di:0.60 */800.70,-1,1.69,1,2,14,12,-1,1,6,14,4,3,0,0,0,0,0,
/* 163,fi:0.0024,di:0.60 */248.36,-1,1.69,1,4,10,8,-1,1,6,10,4,2,0,0,0,0,0,
/* 164,fi:0.0024,di:0.60 */-213.13,1,1.69,3,1,14,6,-1,3,3,14,2,3,0,0,0,0,0,
/* 165,fi:0.0024,di:0.60 */560.78,-1,1.69,0,3,6,12,-1,0,7,6,4,3,0,0,0,0,0,
/* 166,fi:0.0024,di:0.60 */590.10,-1,1.69,9,2,10,8,-1,9,6,10,4,2,0,0,0,0,0,
/* 167,fi:0.0024,di:0.60 */128.79,-1,1.69,9,4,8,4,-1,9,6,8,2,2,0,0,0,0,0,
/* 168,fi:0.0024,di:0.60 */204.38,-1,1.69,7,4,10,8,-1,7,6,10,4,2,0,0,0,0,0,
/* 169,fi:0.0024,di:0.60 */748.17,-1,1.69,0,3,10,12,-1,0,7,10,4,3,0,0,0,0,0,
/* 170,fi:0.0024,di:0.59 */-214.11,1,1.69,4,1,14,6,-1,4,3,14,2,3,0,0,0,0,0,
/* 171,fi:0.0024,di:0.59 */145.62,-1,1.69,9,4,8,8,-1,9,6,8,4,2,0,0,0,0,0,
/* 172,fi:0.0024,di:0.59 */860.03,-1,1.69,5,2,14,8,-1,5,6,14,4,2,0,0,0,0,0,
/* 173,fi:0.0024,di:0.59 */-252.90,1,1.69,10,0,8,8,-1,10,2,8,4,2,0,0,0,0,0,
/* 174,fi:0.0022,di:0.59 */157.42,-1,1.69,7,2,6,4,-1,9,2,2,4,3,0,0,0,0,0,
/* 175,fi:0.0022,di:0.59 */801.11,-1,1.69,3,3,16,8,-1,3,7,16,4,2,0,0,0,0,0,
/* 176,fi:0.0022,di:0.59 */205.18,-1,1.69,0,4,12,4,-1,0,6,12,2,2,0,0,0,0,0,
/* 177,fi:0.0022,di:0.59 */204.39,-1,1.69,6,4,10,8,-1,6,6,10,4,2,0,0,0,0,0,
/* 178,fi:0.0022,di:0.59 */572.82,-1,1.68,9,2,10,12,-1,9,6,10,4,3,0,0,0,0,0,
/* 179,fi:0.0022,di:0.59 */-229.66,1,1.68,2,1,16,6,-1,2,3,16,2,3,0,0,0,0,0,
/* 180,fi:0.0022,di:0.59 */-450.10,1,1.68,5,0,14,8,-1,5,2,14,4,2,0,0,0,0,0,
/* 181,fi:0.0022,di:0.59 */90.95,-1,1.68,11,4,6,4,-1,11,6,6,2,2,0,0,0,0,0,
/* 182,fi:0.0022,di:0.59 */764.71,-1,1.68,1,3,16,8,-1,1,7,16,4,2,0,0,0,0,0,
/* 183,fi:0.0022,di:0.59 */778.08,-1,1.68,3,3,12,12,-1,3,7,12,4,3,0,0,0,0,0,
/* 184,fi:0.0022,di:0.59 */894.85,-1,1.68,0,3,18,8,-1,0,7,18,4,2,0,0,0,0,0,
/* 185,fi:0.0022,di:0.58 */497.35,-1,1.68,13,2,6,8,-1,13,6,6,4,2,0,0,0,0,0,
/* 186,fi:0.0022,di:0.58 */142.69,-1,1.68,8,4,8,8,-1,8,6,8,4,2,0,0,0,0,0,
/* 187,fi:0.0022,di:0.58 */930.15,-1,1.68,7,3,12,12,-1,7,7,12,4,3,0,0,0,0,0,
/* 188,fi:0.0022,di:0.58 */530.24,-1,1.68,11,2,8,8,-1,11,6,8,4,2,0,0,0,0,0,
/* 189,fi:0.0022,di:0.58 */343.84,-1,1.68,13,3,4,8,-1,13,7,4,4,2,0,0,0,0,0,
/* 190,fi:0.0022,di:0.58 */673.29,-1,1.68,0,3,8,12,-1,0,7,8,4,3,0,0,0,0,0,
/* 191,fi:0.0022,di:0.58 */-404.69,1,1.68,5,0,12,8,-1,5,2,12,4,2,0,0,0,0,0,
/* 192,fi:0.0022,di:0.58 */173.99,-1,1.67,14,2,2,8,-1,14,6,2,4,2,0,0,0,0,0,
/* 193,fi:0.0022,di:0.58 */648.73,-1,1.67,5,2,12,12,-1,5,6,12,4,3,0,0,0,0,0,
/* 194,fi:0.0022,di:0.58 */-219.54,1,1.67,1,1,18,6,-1,1,3,18,2,3,0,0,0,0,0,
/* 195,fi:0.0022,di:0.58 */485.53,-1,1.67,10,2,8,8,-1,10,6,8,4,2,0,0,0,0,0,
/* 196,fi:0.0022,di:0.58 */-304.39,1,1.67,8,0,10,8,-1,8,2,10,4,2,0,0,0,0,0,
/* 197,fi:0.0022,di:0.58 */-379.70,1,1.67,6,0,12,8,-1,6,2,12,4,2,0,0,0,0,0,
/* 198,fi:0.0022,di:0.58 */174.04,-1,1.67,2,4,10,4,-1,2,6,10,2,2,0,0,0,0,0,
/* 199,fi:0.0022,di:0.58 */123.94,-1,1.67,4,4,10,4,-1,4,6,10,2,2,0,0,0,0,0,
/* 200,fi:0.0022,di:0.58 */313.57,-1,1.67,2,5,16,8,-1,2,7,16,4,2,0,0,0,0,0,
/* 201,fi:0.0022,di:0.57 */133.94,-1,1.67,10,4,8,8,-1,10,6,8,4,2,0,0,0,0,0,
/* 202,fi:0.0022,di:0.57 */862.32,-1,1.67,0,3,14,12,-1,0,7,14,4,3,0,0,0,0,0,
/* 203,fi:0.0022,di:0.57 */790.19,-1,1.67,0,3,12,12,-1,0,7,12,4,3,0,0,0,0,0,
/* 204,fi:0.0022,di:0.57 */327.70,-1,1.67,13,2,4,8,-1,13,6,4,4,2,0,0,0,0,0,
/* 205,fi:0.0022,di:0.57 */851.05,-1,1.66,6,3,12,12,-1,6,7,12,4,3,0,0,0,0,0,
/* 206,fi:0.0022,di:0.57 */130.34,-1,1.66,5,4,8,8,-1,5,6,8,4,2,0,0,0,0,0,
/* 207,fi:0.0022,di:0.57 */148.42,-1,1.66,7,4,10,4,-1,7,6,10,2,2,0,0,0,0,0,
/* 208,fi:0.0022,di:0.57 */475.55,-1,1.66,13,3,6,8,-1,13,7,6,4,2,0,0,0,0,0,
/* 209,fi:0.0022,di:0.57 */435.22,-1,1.66,12,2,6,8,-1,12,6,6,4,2,0,0,0,0,0,
/* 210,fi:0.0022,di:0.57 */150.18,-1,1.66,3,4,10,4,-1,3,6,10,2,2,0,0,0,0,0,
/* 211,fi:0.0022,di:0.57 */355.82,-1,1.66,9,2,8,12,-1,9,6,8,4,3,0,0,0,0,0,
/* 212,fi:0.0022,di:0.57 */725.25,-1,1.66,2,3,12,12,-1,2,7,12,4,3,0,0,0,0,0,
/* 213,fi:0.0020,di:0.57 */114.42,-1,1.66,7,6,6,6,-1,9,6,2,6,3,0,0,0,0,0,
/* 214,fi:0.0020,di:0.57 */329.98,-1,1.66,14,2,4,8,-1,14,6,4,4,2,0,0,0,0,0,
/* 215,fi:0.0020,di:0.57 */510.75,-1,1.66,8,2,10,12,-1,8,6,10,4,3,0,0,0,0,0,
/* 216,fi:0.0020,di:0.57 */-337.01,1,1.66,2,0,12,8,-1,2,2,12,4,2,0,0,0,0,0,
/* 217,fi:0.0020,di:0.57 */-190.45,1,1.66,1,1,16,6,-1,1,3,16,2,3,0,0,0,0,0,
/* 218,fi:0.0020,di:0.57 */360.36,-1,1.66,1,5,18,8,-1,1,7,18,4,2,0,0,0,0,0,
/* 219,fi:0.0020,di:0.57 */157.34,-1,1.66,1,4,10,4,-1,1,6,10,2,2,0,0,0,0,0,
/* 220,fi:0.0020,di:0.57 */104.60,-1,1.66,12,4,6,4,-1,12,6,6,2,2,0,0,0,0,0,
/* 221,fi:0.0020,di:0.57 */-300.54,1,1.65,7,0,10,8,-1,7,2,10,4,2,0,0,0,0,0,
/* 222,fi:0.0020,di:0.56 */141.86,-1,1.65,4,3,2,8,-1,4,7,2,4,2,0,0,0,0,0,
/* 223,fi:0.0020,di:0.56 */84.93,-1,1.65,10,4,6,4,-1,10,6,6,2,2,0,0,0,0,0,
/* 224,fi:0.0020,di:0.56 */-174.18,1,1.65,4,1,12,6,-1,4,3,12,2,3,0,0,0,0,0,
/* 225,fi:0.0020,di:0.56 */433.33,-1,1.65,10,3,8,8,-1,10,7,8,4,2,0,0,0,0,0,
/* 226,fi:0.0020,di:0.56 */631.39,-1,1.65,3,3,14,8,-1,3,7,14,4,2,0,0,0,0,0,
/* 227,fi:0.0020,di:0.56 */-175.95,1,1.65,5,1,14,6,-1,5,3,14,2,3,0,0,0,0,0,
/* 228,fi:0.0020,di:0.56 */757.74,-1,1.65,1,2,14,8,-1,1,6,14,4,2,0,0,0,0,0,
/* 229,fi:0.0020,di:0.56 */421.45,-1,1.65,12,3,6,8,-1,12,7,6,4,2,0,0,0,0,0,
/* 230,fi:0.0020,di:0.56 */594.08,-1,1.65,6,2,12,12,-1,6,6,12,4,3,0,0,0,0,0,
/* 231,fi:0.0020,di:0.56 */676.12,-1,1.65,4,2,12,8,-1,4,6,12,4,2,0,0,0,0,0,
/* 232,fi:0.0020,di:0.56 */665.20,-1,1.65,2,3,10,12,-1,2,7,10,4,3,0,0,0,0,0,
/* 233,fi:0.0020,di:0.56 */639.54,-1,1.65,7,2,12,12,-1,7,6,12,4,3,0,0,0,0,0,
/* 234,fi:0.0020,di:0.56 */331.10,-1,1.65,15,3,4,8,-1,15,7,4,4,2,0,0,0,0,0,
/* 235,fi:0.0020,di:0.56 */488.65,-1,1.65,3,3,6,12,-1,3,7,6,4,3,0,0,0,0,0,
/* 236,fi:0.0020,di:0.56 */331.59,-1,1.65,0,5,18,8,-1,0,7,18,4,2,0,0,0,0,0,
/* 237,fi:0.0020,di:0.55 */192.19,-1,1.65,7,5,6,12,-1,9,5,2,12,3,0,0,0,0,0,
/* 238,fi:0.0020,di:0.55 */742.91,-1,1.65,1,3,12,12,-1,1,7,12,4,3,0,0,0,0,0,
/* 239,fi:0.0020,di:0.55 */268.73,-1,1.65,1,5,16,8,-1,1,7,16,4,2,0,0,0,0,0,
/* 240,fi:0.0020,di:0.55 */-268.01,1,1.64,6,0,10,8,-1,6,2,10,4,2,0,0,0,0,0,
/* 241,fi:0.0020,di:0.55 */-176.79,1,1.64,10,0,6,8,-1,10,2,6,4,2,0,0,0,0,0,
/* 242,fi:0.0020,di:0.55 */594.80,-1,1.65,7,3,10,12,-1,7,7,10,4,3,0,0,0,0,0,
/* 243,fi:0.0020,di:0.55 */317.27,-1,1.65,3,5,16,8,-1,3,7,16,4,2,0,0,0,0,0,
/* 244,fi:0.0020,di:0.55 */-185.03,1,1.64,2,1,14,6,-1,2,3,14,2,3,0,0,0,0,0,
/* 245,fi:0.0020,di:0.55 */-155.50,1,1.64,5,1,12,6,-1,5,3,12,2,3,0,0,0,0,0,
/* 246,fi:0.0020,di:0.55 */192.94,-1,1.64,2,4,8,8,-1,2,6,8,4,2,0,0,0,0,0,
/* 247,fi:0.0020,di:0.55 */-348.62,1,1.64,7,0,12,8,-1,7,2,12,4,2,0,0,0,0,0,
/* 248,fi:0.0020,di:0.55 */379.40,-1,1.64,10,3,6,12,-1,10,7,6,4,3,0,0,0,0,0,
/* 249,fi:0.0020,di:0.55 */37.65,-1,1.64,7,4,6,2,-1,9,4,2,2,3,0,0,0,0,0,
/* 250,fi:0.0020,di:0.55 */-199.12,1,1.65,0,1,18,6,-1,0,3,18,2,3,0,0,0,0,0,
/* 251,fi:0.0020,di:0.55 */474.81,-1,1.65,11,3,8,8,-1,11,7,8,4,2,0,0,0,0,0,
/* 252,fi:0.0020,di:0.55 */305.31,-1,1.65,2,3,4,8,-1,2,7,4,4,2,0,0,0,0,0,
/* 253,fi:0.0020,di:0.54 */-258.07,1,1.65,4,0,10,8,-1,4,2,10,4,2,0,0,0,0,0,
/* 254,fi:0.0020,di:0.54 */674.55,-1,1.65,3,2,12,12,-1,3,6,12,4,3,0,0,0,0,0,
/* 255,fi:0.0020,di:0.54 */660.65,-1,1.65,4,3,14,8,-1,4,7,14,4,2,0,0,0,0,0,
/* 256,fi:0.0020,di:0.54 */-222.19,1,1.65,5,0,10,8,-1,5,2,10,4,2,0,0,0,0,0,
/* 257,fi:0.0020,di:0.54 */-178.92,1,1.64,11,0,6,8,-1,11,2,6,4,2,0,0,0,0,0,
/* 258,fi:0.0020,di:0.54 */188.26,-1,1.64,2,3,2,12,-1,2,7,2,4,3,0,0,0,0,0,
/* 259,fi:0.0020,di:0.54 */119.10,-1,1.64,6,4,10,4,-1,6,6,10,2,2,0,0,0,0,0,
/* 260,fi:0.0020,di:0.54 */68.44,-1,1.64,13,4,4,4,-1,13,6,4,2,2,0,0,0,0,0,
/* 261,fi:0.0020,di:0.54 */-332.70,1,1.64,0,0,14,8,-1,0,2,14,4,2,0,0,0,0,0,
/* 262,fi:0.0020,di:0.54 */160.73,-1,1.64,4,2,2,8,-1,4,6,2,4,2,0,0,0,0,0,
/* 263,fi:0.0020,di:0.54 */336.64,-1,1.64,12,3,4,12,-1,12,7,4,4,3,0,0,0,0,0,
/* 264,fi:0.0020,di:0.54 */105.65,-1,1.64,5,4,10,4,-1,5,6,10,2,2,0,0,0,0,0,
/* 265,fi:0.0020,di:0.54 */120.29,-1,1.64,13,4,6,4,-1,13,6,6,2,2,0,0,0,0,0,
/* 266,fi:0.0018,di:0.53 */65.33,-1,1.64,7,5,6,2,-1,9,5,2,2,3,0,0,0,0,0,
// end of 9-th stage with 267 classifiers
/* 267,fi:0.0018,di:0.53 */-315.70,1,1.64,1,0,12,8,-1,1,2,12,4,2,0,0,0,0,0,
/* 268,fi:0.0018,di:0.53 */80.77,-1,1.64,9,4,6,8,-1,9,6,6,4,2,0,0,0,0,0,
/* 269,fi:0.0018,di:0.53 */-233.79,1,1.64,9,0,8,8,-1,9,2,8,4,2,0,0,0,0,0,
/* 270,fi:0.0018,di:0.53 */81.93,-1,1.64,10,4,6,8,-1,10,6,6,4,2,0,0,0,0,0,
/* 271,fi:0.0018,di:0.53 */-138.54,1,1.64,3,1,12,6,-1,3,3,12,2,3,0,0,0,0,0,
/* 272,fi:0.0018,di:0.53 */-269.77,1,1.64,3,0,10,8,-1,3,2,10,4,2,0,0,0,0,0,
/* 273,fi:0.0018,di:0.53 */577.26,-1,1.64,3,3,8,12,-1,3,7,8,4,3,0,0,0,0,0,
/* 274,fi:0.0018,di:0.53 */103.01,-1,1.64,7,4,8,8,-1,7,6,8,4,2,0,0,0,0,0,
/* 275,fi:0.0018,di:0.53 */651.61,-1,1.64,5,2,12,8,-1,5,6,12,4,2,0,0,0,0,0,
/* 276,fi:0.0018,di:0.53 */359.40,-1,1.64,11,2,6,8,-1,11,6,6,4,2,0,0,0,0,0,
/* 277,fi:0.0018,di:0.53 */233.41,-1,1.64,3,5,14,8,-1,3,7,14,4,2,0,0,0,0,0,
/* 278,fi:0.0018,di:0.53 */575.97,-1,1.64,2,3,14,8,-1,2,7,14,4,2,0,0,0,0,0,
/* 279,fi:0.0018,di:0.53 */-167.00,1,1.64,0,1,16,6,-1,0,3,16,2,3,0,0,0,0,0,
/* 280,fi:0.0018,di:0.53 */-157.24,1,1.63,6,1,12,6,-1,6,3,12,2,3,0,0,0,0,0,
/* 281,fi:0.0018,di:0.53 */87.13,-1,1.63,8,4,8,4,-1,8,6,8,2,2,0,0,0,0,0,
/* 282,fi:0.0018,di:0.53 */322.65,-1,1.63,10,2,8,12,-1,10,6,8,4,3,0,0,0,0,0,
/* 283,fi:0.0018,di:0.53 */398.12,-1,1.63,8,3,8,12,-1,8,7,8,4,3,0,0,0,0,0,
/* 284,fi:0.0018,di:0.53 */202.98,-1,1.63,2,5,14,8,-1,2,7,14,4,2,0,0,0,0,0,
/* 285,fi:0.0018,di:0.53 */244.97,-1,1.63,2,6,16,6,-1,2,8,16,2,3,0,0,0,0,0,
/* 286,fi:0.0018,di:0.52 */160.35,-1,1.63,15,2,2,8,-1,15,6,2,4,2,0,0,0,0,0,
/* 287,fi:0.0018,di:0.52 */51.47,-1,1.63,12,4,4,4,-1,12,6,4,2,2,0,0,0,0,0,
/* 288,fi:0.0018,di:0.52 */106.84,-1,1.63,11,4,8,8,-1,11,6,8,4,2,0,0,0,0,0,
/* 289,fi:0.0018,di:0.52 */168.11,-1,1.63,4,4,6,8,-1,4,6,6,4,2,0,0,0,0,0,
/* 290,fi:0.0018,di:0.52 */662.39,-1,1.63,0,3,16,8,-1,0,7,16,4,2,0,0,0,0,0,
/* 291,fi:0.0018,di:0.52 */156.56,-1,1.63,0,4,10,4,-1,0,6,10,2,2,0,0,0,0,0,
/* 292,fi:0.0018,di:0.52 */322.49,-1,1.63,11,3,6,8,-1,11,7,6,4,2,0,0,0,0,0,
/* 293,fi:0.0018,di:0.52 */-270.76,1,1.63,2,0,10,8,-1,2,2,10,4,2,0,0,0,0,0,
/* 294,fi:0.0018,di:0.52 */-208.95,1,1.63,3,0,6,8,-1,3,2,6,4,2,0,0,0,0,0,
/* 295,fi:0.0018,di:0.52 */626.64,-1,1.63,2,2,12,12,-1,2,6,12,4,3,0,0,0,0,0,
/* 296,fi:0.0018,di:0.52 */127.01,-1,1.62,1,4,8,4,-1,1,6,8,2,2,0,0,0,0,0,
/* 297,fi:0.0015,di:0.52 */261.39,-1,1.62,16,3,2,12,-1,16,7,2,4,3,0,0,0,0,0,
/* 298,fi:0.0015,di:0.52 */385.75,-1,1.62,5,2,10,12,-1,5,6,10,4,3,0,0,0,0,0,
/* 299,fi:0.0013,di:0.52 */35.04,-1,1.62,7,3,6,2,-1,9,3,2,2,3,0,0,0,0,0,
/* 300,fi:0.0013,di:0.52 */749.21,-1,1.62,0,2,14,12,-1,0,6,14,4,3,0,0,0,0,0,
/* 301,fi:0.0013,di:0.52 */228.57,-1,1.62,0,5,16,8,-1,0,7,16,4,2,0,0,0,0,0,
/* 302,fi:0.0013,di:0.52 */630.73,-1,1.62,3,2,12,8,-1,3,6,12,4,2,0,0,0,0,0,
/* 303,fi:0.0013,di:0.52 */645.33,-1,1.62,7,2,12,8,-1,7,6,12,4,2,0,0,0,0,0,
/* 304,fi:0.0013,di:0.52 */248.46,-1,1.62,4,5,14,8,-1,4,7,14,4,2,0,0,0,0,0,
/* 305,fi:0.0013,di:0.52 */212.15,-1,1.62,0,4,10,8,-1,0,6,10,4,2,0,0,0,0,0,
/* 306,fi:0.0013,di:0.52 */110.98,-1,1.62,6,4,8,8,-1,6,6,8,4,2,0,0,0,0,0,
/* 307,fi:0.0011,di:0.52 */251.77,-1,1.62,3,6,16,6,-1,3,8,16,2,3,0,0,0,0,0,
// end of 10-th stage with 308 classifiers
/* 308,fi:0.0011,di:0.52 */541.82,-1,1.61,8,2,10,8,-1,8,6,10,4,2,0,0,0,0,0,
/* 309,fi:0.0011,di:0.52 */209.51,-1,1.61,1,5,18,4,-1,1,7,18,2,2,0,0,0,0,0,
/* 310,fi:0.0011,di:0.52 */479.15,-1,1.61,9,3,10,8,-1,9,7,10,4,2,0,0,0,0,0,
/* 311,fi:0.0011,di:0.51 */277.94,-1,1.61,1,3,4,8,-1,1,7,4,4,2,0,0,0,0,0,
/* 312,fi:0.0011,di:0.51 */-132.46,1,1.61,7,1,12,6,-1,7,3,12,2,3,0,0,0,0,0,
/* 313,fi:0.0011,di:0.51 */-154.77,1,1.61,1,1,14,6,-1,1,3,14,2,3,0,0,0,0,0,
/* 314,fi:0.0011,di:0.51 */664.61,-1,1.61,0,2,14,8,-1,0,6,14,4,2,0,0,0,0,0,
/* 315,fi:0.0011,di:0.51 */210.32,-1,1.61,3,6,14,6,-1,3,8,14,2,3,0,0,0,0,0,
/* 316,fi:0.0011,di:0.51 */128.80,-1,1.61,2,3,2,8,-1,2,7,2,4,2,0,0,0,0,0,
/* 317,fi:0.0011,di:0.50 */-202.82,1,1.61,9,1,8,10,-1,11,1,4,10,2,0,0,0,0,0,
/* 318,fi:0.0011,di:0.50 */-212.79,1,1.61,8,0,8,8,-1,8,2,8,4,2,0,0,0,0,0,
/* 319,fi:0.0011,di:0.50 */605.64,-1,1.61,5,3,14,8,-1,5,7,14,4,2,0,0,0,0,0,
/* 320,fi:0.0011,di:0.50 */383.29,-1,1.61,1,3,6,8,-1,1,7,6,4,2,0,0,0,0,0,
/* 321,fi:0.0011,di:0.50 */-272.02,1,1.61,9,0,10,8,-1,9,2,10,4,2,0,0,0,0,0,
/* 322,fi:0.0011,di:0.50 */-209.60,1,1.61,11,0,8,8,-1,11,2,8,4,2,0,0,0,0,0,
/* 323,fi:0.0011,di:0.50 */250.86,-1,1.61,3,3,4,8,-1,3,7,4,4,2,0,0,0,0,0,
/* 324,fi:0.0011,di:0.50 */160.14,-1,1.61,2,5,10,8,-1,2,7,10,4,2,0,0,0,0,0,
/* 325,fi:0.0011,di:0.50 */266.54,-1,1.61,8,2,8,12,-1,8,6,8,4,3,0,0,0,0,0,
/* 326,fi:0.0011,di:0.49 */167.56,-1,1.61,16,3,2,8,-1,16,7,2,4,2,0,0,0,0,0,
/* 327,fi:0.0011,di:0.49 */202.63,-1,1.61,9,5,10,8,-1,9,7,10,4,2,0,0,0,0,0,
/* 328,fi:0.0011,di:0.49 */166.32,-1,1.61,13,3,2,12,-1,13,7,2,4,3,0,0,0,0,0,
/* 329,fi:0.0011,di:0.49 */138.28,-1,1.61,5,4,6,8,-1,5,6,6,4,2,0,0,0,0,0,
/* 330,fi:0.0011,di:0.49 */160.68,-1,1.61,7,5,6,10,-1,9,5,2,10,3,0,0,0,0,0,
/* 331,fi:0.0011,di:0.49 */224.21,-1,1.60,7,0,6,6,-1,9,0,2,6,3,0,0,0,0,0,
/* 332,fi:0.0011,di:0.49 */205.58,-1,1.60,0,5,18,4,-1,0,7,18,2,2,0,0,0,0,0,
/* 333,fi:0.0011,di:0.49 */256.29,-1,1.60,1,6,16,6,-1,1,8,16,2,3,0,0,0,0,0,
/* 334,fi:0.0011,di:0.49 */214.71,-1,1.60,2,6,14,6,-1,2,8,14,2,3,0,0,0,0,0,
/* 335,fi:0.0011,di:0.49 */185.87,-1,1.60,2,6,10,6,-1,2,8,10,2,3,0,0,0,0,0,
/* 336,fi:0.0011,di:0.49 */181.83,-1,1.60,1,5,16,4,-1,1,7,16,2,2,0,0,0,0,0,
/* 337,fi:0.0011,di:0.49 */-57.78,1,1.60,9,2,4,4,-1,11,2,2,4,2,0,0,0,0,0,
/* 338,fi:0.0007,di:0.48 */71.33,-1,1.62,7,7,6,4,-1,9,7,2,4,3,0,0,0,0,0,
// end of 11-th stage with 339 classifiers
/* 339,fi:0.0007,di:0.48 */159.09,-1,1.60,3,6,8,6,-1,3,8,8,2,3,0,0,0,0,0,
/* 340,fi:0.0007,di:0.48 */183.14,-1,1.60,1,6,10,6,-1,1,8,10,2,3,0,0,0,0,0,
/* 341,fi:0.0007,di:0.48 */205.71,-1,1.60,4,6,14,6,-1,4,8,14,2,3,0,0,0,0,0,
/* 342,fi:0.0007,di:0.48 */184.00,-1,1.60,3,6,10,6,-1,3,8,10,2,3,0,0,0,0,0,
/* 343,fi:0.0007,di:0.48 */-140.77,1,1.60,9,3,8,8,-1,11,3,4,8,2,0,0,0,0,0,
/* 344,fi:0.0007,di:0.48 */191.69,-1,1.60,1,5,10,8,-1,1,7,10,4,2,0,0,0,0,0,
/* 345,fi:0.0007,di:0.48 */-196.82,1,1.59,4,0,8,8,-1,4,2,8,4,2,0,0,0,0,0,
/* 346,fi:0.0007,di:0.48 */-167.47,1,1.59,9,2,4,8,-1,11,2,2,8,2,0,0,0,0,0,
/* 347,fi:0.0007,di:0.48 */64.00,-1,1.59,14,4,4,4,-1,14,6,4,2,2,0,0,0,0,0,
/* 348,fi:0.0007,di:0.48 */399.55,-1,1.59,9,2,8,8,-1,9,6,8,4,2,0,0,0,0,0,
/* 349,fi:0.0007,di:0.48 */-121.97,1,1.59,9,1,4,6,-1,11,1,2,6,2,0,0,0,0,0,
/* 350,fi:0.0007,di:0.48 */274.77,-1,1.59,1,6,18,6,-1,1,8,18,2,3,0,0,0,0,0,
/* 351,fi:0.0007,di:0.47 */151.09,-1,1.59,8,6,10,6,-1,8,8,10,2,3,0,0,0,0,0,
/* 352,fi:0.0007,di:0.47 */-162.67,1,1.59,12,0,6,8,-1,12,2,6,4,2,0,0,0,0,0,
/* 353,fi:0.0007,di:0.47 */-148.60,1,1.59,7,0,8,8,-1,7,2,8,4,2,0,0,0,0,0,
/* 354,fi:0.0007,di:0.47 */31.97,-1,1.59,14,4,2,4,-1,14,6,2,2,2,0,0,0,0,0,
/* 355,fi:0.0007,di:0.47 */297.03,-1,1.60,15,2,4,8,-1,15,6,4,4,2,0,0,0,0,0,
/* 356,fi:0.0007,di:0.47 */186.61,-1,1.60,7,6,12,6,-1,7,8,12,2,3,0,0,0,0,0,
/* 357,fi:0.0007,di:0.46 */699.86,-1,1.60,13,2,6,16,-1,13,6,6,8,2,0,0,0,0,0,
/* 358,fi:0.0007,di:0.46 */194.60,-1,1.60,2,5,16,4,-1,2,7,16,2,2,0,0,0,0,0,
/* 359,fi:0.0007,di:0.46 */438.10,-1,1.59,8,3,10,8,-1,8,7,10,4,2,0,0,0,0,0,
/* 360,fi:0.0004,di:0.46 */-225.06,1,1.59,3,0,8,8,-1,3,2,8,4,2,0,0,0,0,0,
// end of 12-th stage with 361 classifiers
/* 361,fi:0.0004,di:0.46 */509.08,-1,1.59,3,3,10,12,-1,3,7,10,4,3,0,0,0,0,0,
/* 362,fi:0.0004,di:0.46 */264.34,-1,1.59,2,4,4,12,-1,2,8,4,4,3,0,0,0,0,0,
/* 363,fi:0.0004,di:0.46 */431.72,-1,1.59,6,3,10,12,-1,6,7,10,4,3,0,0,0,0,0,
/* 364,fi:0.0004,di:0.46 */842.91,-1,1.59,14,1,4,18,-1,14,7,4,6,3,0,0,0,0,0,
/* 365,fi:0.0004,di:0.46 */185.83,-1,1.59,3,5,16,4,-1,3,7,16,2,2,0,0,0,0,0,
/* 366,fi:0.0004,di:0.46 */277.20,-1,1.59,4,3,4,12,-1,4,7,4,4,3,0,0,0,0,0,
/* 367,fi:0.0004,di:0.46 */-108.09,1,1.59,5,0,8,8,-1,5,2,8,4,2,0,0,0,0,0,
/* 368,fi:0.0004,di:0.46 */197.72,-1,1.59,1,6,12,6,-1,1,8,12,2,3,0,0,0,0,0,
/* 369,fi:0.0004,di:0.46 */259.03,-1,1.59,5,5,14,8,-1,5,7,14,4,2,0,0,0,0,0,
/* 370,fi:0.0004,di:0.46 */123.59,-1,1.59,4,4,2,12,-1,4,8,2,4,3,0,0,0,0,0,
/* 371,fi:0.0004,di:0.45 */757.11,-1,1.59,15,1,4,18,-1,15,7,4,6,3,0,0,0,0,0,
/* 372,fi:0.0004,di:0.45 */-264.37,1,1.59,0,0,12,8,-1,0,2,12,4,2,0,0,0,0,0,
/* 373,fi:0.0004,di:0.45 */185.42,-1,1.59,2,6,12,6,-1,2,8,12,2,3,0,0,0,0,0,
/* 374,fi:0.0004,di:0.45 */209.66,-1,1.59,0,6,12,6,-1,0,8,12,2,3,0,0,0,0,0,
/* 375,fi:0.0004,di:0.45 */393.48,-1,1.59,0,3,6,8,-1,0,7,6,4,2,0,0,0,0,0,
/* 376,fi:0.0004,di:0.45 */210.37,-1,1.59,5,6,14,6,-1,5,8,14,2,3,0,0,0,0,0,
/* 377,fi:0.0004,di:0.45 */250.45,-1,1.59,11,2,8,12,-1,11,6,8,4,3,0,0,0,0,0,
/* 378,fi:0.0004,di:0.45 */24.73,1,1.59,1,2,18,6,-1,1,4,18,2,3,0,0,0,0,0,
/* 379,fi:0.0004,di:0.45 */-227.80,1,1.59,2,0,8,8,-1,2,2,8,4,2,0,0,0,0,0,
/* 380,fi:0.0004,di:0.45 */-247.46,1,1.59,1,0,10,8,-1,1,2,10,4,2,0,0,0,0,0,
/* 381,fi:0.0004,di:0.45 */104.93,-1,1.59,3,4,6,4,-1,3,6,6,2,2,0,0,0,0,0,
/* 382,fi:0.0004,di:0.45 */-118.24,1,1.58,3,1,10,6,-1,3,3,10,2,3,0,0,0,0,0,
/* 383,fi:0.0004,di:0.45 */626.45,-1,1.58,6,2,12,8,-1,6,6,12,4,2,0,0,0,0,0,
/* 384,fi:0.0004,di:0.45 */369.88,-1,1.58,6,2,10,12,-1,6,6,10,4,3,0,0,0,0,0,
/* 385,fi:0.0004,di:0.45 */188.44,-1,1.58,1,5,14,8,-1,1,7,14,4,2,0,0,0,0,0,
/* 386,fi:0.0004,di:0.45 */140.33,-1,1.58,4,6,10,6,-1,4,8,10,2,3,0,0,0,0,0,
/* 387,fi:0.0004,di:0.45 */164.92,-1,1.58,4,6,12,6,-1,4,8,12,2,3,0,0,0,0,0,
/* 388,fi:0.0004,di:0.45 */117.00,-1,1.58,2,4,8,4,-1,2,6,8,2,2,0,0,0,0,0,
/* 389,fi:0.0004,di:0.45 */-76.75,1,1.58,11,0,4,8,-1,11,2,4,4,2,0,0,0,0,0,
/* 390,fi:0.0004,di:0.45 */380.40,-1,1.58,4,3,6,12,-1,4,7,6,4,3,0,0,0,0,0,
/* 391,fi:0.0004,di:0.45 */-214.01,1,1.58,1,0,8,8,-1,1,2,8,4,2,0,0,0,0,0,
/* 392,fi:0.0004,di:0.45 */145.37,-1,1.58,4,6,8,6,-1,4,8,8,2,3,0,0,0,0,0,
/* 393,fi:0.0004,di:0.45 */76.07,-1,1.58,1,4,6,4,-1,1,6,6,2,2,0,0,0,0,0,
/* 394,fi:0.0004,di:0.45 */184.79,-1,1.58,5,6,12,6,-1,5,8,12,2,3,0,0,0,0,0,
/* 395,fi:0.0004,di:0.44 */34.28,-1,1.58,4,4,2,4,-1,4,6,2,2,2,0,0,0,0,0,
/* 396,fi:0.0004,di:0.44 */27.31,1,1.59,2,2,16,6,-1,2,4,16,2,3,0,0,0,0,0,
/* 397,fi:0.0004,di:0.44 */1113.08,-1,1.59,13,1,6,18,-1,13,7,6,6,3,0,0,0,0,0,
/* 398,fi:0.0004,di:0.44 */673.67,-1,1.59,15,0,4,18,-1,15,6,4,6,3,0,0,0,0,0,
/* 399,fi:0.0004,di:0.44 */230.87,-1,1.58,0,6,16,6,-1,0,8,16,2,3,0,0,0,0,0,
/* 400,fi:0.0004,di:0.44 */-65.62,1,1.58,4,0,2,8,-1,4,2,2,4,2,0,0,0,0,0,
/* 401,fi:0.0004,di:0.44 */177.88,-1,1.58,6,6,12,6,-1,6,8,12,2,3,0,0,0,0,0,
/* 402,fi:0.0004,di:0.44 */260.78,-1,1.58,3,2,4,8,-1,3,6,4,4,2,0,0,0,0,0,
/* 403,fi:0.0004,di:0.44 */209.72,-1,1.58,1,6,14,6,-1,1,8,14,2,3,0,0,0,0,0,
/* 404,fi:0.0004,di:0.44 */185.70,-1,1.58,8,5,10,8,-1,8,7,10,4,2,0,0,0,0,0,
/* 405,fi:0.0004,di:0.44 */-127.67,1,1.58,4,0,4,8,-1,4,2,4,4,2,0,0,0,0,0,
/* 406,fi:0.0004,di:0.44 */270.19,-1,1.58,0,6,18,6,-1,0,8,18,2,3,0,0,0,0,0,
/* 407,fi:0.0004,di:0.44 */327.86,-1,1.58,2,3,6,8,-1,2,7,6,4,2,0,0,0,0,0,
/* 408,fi:0.0004,di:0.44 */161.55,-1,1.58,7,6,10,6,-1,7,8,10,2,3,0,0,0,0,0,
/* 409,fi:0.0004,di:0.44 */131.99,-1,1.58,3,4,6,8,-1,3,6,6,4,2,0,0,0,0,0,
/* 410,fi:0.0004,di:0.44 */157.10,-1,1.58,10,2,6,12,-1,10,6,6,4,3,0,0,0,0,0,
/* 411,fi:0.0004,di:0.43 */182.44,-1,1.58,7,5,6,14,-1,9,5,2,14,3,0,0,0,0,0,
/* 412,fi:0.0004,di:0.43 */176.52,-1,1.57,3,6,12,6,-1,3,8,12,2,3,0,0,0,0,0,
/* 413,fi:0.0004,di:0.43 */-242.49,1,1.57,2,2,8,14,-1,4,2,4,14,2,0,0,0,0,0,
/* 414,fi:0.0004,di:0.43 */603.28,-1,1.57,1,2,12,12,-1,1,6,12,4,3,0,0,0,0,0,
/* 415,fi:0.0004,di:0.42 */18.70,1,1.57,1,2,16,6,-1,1,4,16,2,3,0,0,0,0,0,
/* 416,fi:0.0004,di:0.42 */118.98,-1,1.57,0,4,8,4,-1,0,6,8,2,2,0,0,0,0,0,
/* 417,fi:0.0004,di:0.42 */45.60,1,1.57,0,2,18,6,-1,0,4,18,2,3,0,0,0,0,0,
/* 418,fi:0.0004,di:0.42 */52.71,-1,1.57,2,5,4,4,-1,2,7,4,2,2,0,0,0,0,0,
/* 419,fi:0.0004,di:0.42 */81.21,-1,1.57,2,4,6,4,-1,2,6,6,2,2,0,0,0,0,0,
/* 420,fi:0.0004,di:0.42 */424.76,-1,1.57,4,2,10,12,-1,4,6,10,4,3,0,0,0,0,0,
/* 421,fi:0.0004,di:0.42 */409.84,-1,1.57,7,2,10,12,-1,7,6,10,4,3,0,0,0,0,0,
/* 422,fi:0.0004,di:0.42 */56.07,-1,1.57,11,4,6,8,-1,11,6,6,4,2,0,0,0,0,0,
/* 423,fi:0.0004,di:0.42 */-190.50,1,1.57,9,1,4,10,-1,11,1,2,10,2,0,0,0,0,0,
/* 424,fi:0.0004,di:0.42 */486.34,-1,1.57,7,3,12,8,-1,7,7,12,4,2,0,0,0,0,0,
/* 425,fi:0.0004,di:0.42 */129.81,-1,1.57,3,4,8,4,-1,3,6,8,2,2,0,0,0,0,0,
/* 426,fi:0.0004,di:0.42 */496.55,-1,1.57,2,2,10,12,-1,2,6,10,4,3,0,0,0,0,0,
/* 427,fi:0.0004,di:0.42 */313.69,-1,1.57,9,3,8,8,-1,9,7,8,4,2,0,0,0,0,0,
/* 428,fi:0.0004,di:0.42 */500.80,-1,1.57,1,3,14,8,-1,1,7,14,4,2,0,0,0,0,0,
/* 429,fi:0.0004,di:0.42 */235.26,-1,1.57,2,2,4,8,-1,2,6,4,4,2,0,0,0,0,0,
/* 430,fi:0.0004,di:0.42 */646.98,-1,1.57,0,2,12,12,-1,0,6,12,4,3,0,0,0,0,0,
/* 431,fi:0.0004,di:0.42 */121.29,-1,1.57,8,6,8,6,-1,8,8,8,2,3,0,0,0,0,0,
/* 432,fi:0.0004,di:0.42 */151.69,-1,1.57,3,4,2,12,-1,3,8,2,4,3,0,0,0,0,0,
/* 433,fi:0.0004,di:0.42 */160.14,-1,1.57,0,5,16,4,-1,0,7,16,2,2,0,0,0,0,0,
/* 434,fi:0.0004,di:0.42 */952.72,-1,1.57,13,0,6,18,-1,13,6,6,6,3,0,0,0,0,0,
/* 435,fi:0.0004,di:0.42 */137.31,-1,1.57,2,5,14,4,-1,2,7,14,2,2,0,0,0,0,0,
/* 436,fi:0.0004,di:0.42 */162.37,-1,1.57,2,5,8,8,-1,2,7,8,4,2,0,0,0,0,0,
/* 437,fi:0.0004,di:0.42 */110.01,-1,1.57,13,2,2,8,-1,13,6,2,4,2,0,0,0,0,0,
/* 438,fi:0.0004,di:0.42 */1528.66,-1,1.57,1,2,18,16,-1,1,6,18,8,2,0,0,0,0,0,
/* 439,fi:0.0004,di:0.42 */402.23,-1,1.57,1,3,8,8,-1,1,7,8,4,2,0,0,0,0,0,
/* 440,fi:0.0004,di:0.42 */452.88,-1,1.57,15,2,4,16,-1,15,6,4,8,2,0,0,0,0,0,
/* 441,fi:0.0004,di:0.42 */169.42,-1,1.57,0,5,10,8,-1,0,7,10,4,2,0,0,0,0,0,
/* 442,fi:0.0004,di:0.42 */124.22,-1,1.57,3,5,12,8,-1,3,7,12,4,2,0,0,0,0,0,
/* 443,fi:0.0004,di:0.42 */243.81,-1,1.57,12,2,4,8,-1,12,6,4,4,2,0,0,0,0,0,
/* 444,fi:0.0004,di:0.42 */-80.45,1,1.57,1,1,12,6,-1,1,3,12,2,3,0,0,0,0,0,
/* 445,fi:0.0004,di:0.42 */-69.06,1,1.57,3,1,6,6,-1,3,3,6,2,3,0,0,0,0,0,
/* 446,fi:0.0004,di:0.42 */-99.12,1,1.57,2,1,10,6,-1,2,3,10,2,3,0,0,0,0,0,
/* 447,fi:0.0004,di:0.42 */-90.69,1,1.57,8,1,10,6,-1,8,3,10,2,3,0,0,0,0,0,
/* 448,fi:0.0002,di:0.41 */-140.63,1,1.57,4,2,6,10,-1,6,2,2,10,3,0,0,0,0,0,
// end of 13-th stage with 449 classifiers
/* 449,fi:0.0002,di:0.41 */447.01,-1,1.57,3,2,10,12,-1,3,6,10,4,3,0,0,0,0,0,
/* 450,fi:0.0002,di:0.41 */149.75,-1,1.57,3,5,8,8,-1,3,7,8,4,2,0,0,0,0,0,
/* 451,fi:0.0002,di:0.41 */426.56,-1,1.57,0,3,8,8,-1,0,7,8,4,2,0,0,0,0,0,
/* 452,fi:0.0002,di:0.41 */225.68,-1,1.57,7,5,12,8,-1,7,7,12,4,2,0,0,0,0,0,
/* 453,fi:0.0002,di:0.41 */94.99,-1,1.56,10,5,8,4,-1,10,7,8,2,2,0,0,0,0,0,
/* 454,fi:0.0002,di:0.41 */118.13,-1,1.56,4,5,12,8,-1,4,7,12,4,2,0,0,0,0,0,
/* 455,fi:0.0002,di:0.41 */130.11,-1,1.56,9,2,6,12,-1,9,6,6,4,3,0,0,0,0,0,
/* 456,fi:0.0002,di:0.41 */557.91,-1,1.56,1,2,10,12,-1,1,6,10,4,3,0,0,0,0,0,
/* 457,fi:0.0002,di:0.40 */198.47,-1,1.56,12,3,4,8,-1,12,7,4,4,2,0,0,0,0,0,
/* 458,fi:0.0002,di:0.40 */164.20,-1,1.56,0,5,14,8,-1,0,7,14,4,2,0,0,0,0,0,
/* 459,fi:0.0002,di:0.40 */-201.21,1,1.56,9,0,4,10,-1,11,0,2,10,2,0,0,0,0,0,
/* 460,fi:0.0002,di:0.40 */154.24,-1,1.56,3,5,14,4,-1,3,7,14,2,2,0,0,0,0,0,
/* 461,fi:0.0002,di:0.40 */67.32,-1,1.56,3,4,4,4,-1,3,6,4,2,2,0,0,0,0,0,
/* 462,fi:0.0002,di:0.40 */179.55,-1,1.56,14,4,2,12,-1,14,8,2,4,3,0,0,0,0,0,
/* 463,fi:0.0002,di:0.40 */191.00,-1,1.56,0,5,12,8,-1,0,7,12,4,2,0,0,0,0,0,
/* 464,fi:0.0002,di:0.40 */79.20,1,1.56,3,2,16,6,-1,3,4,16,2,3,0,0,0,0,0,
/* 465,fi:0.0002,di:0.40 */-146.43,1,1.56,9,2,8,8,-1,11,2,4,8,2,0,0,0,0,0,
/* 466,fi:0.0002,di:0.40 */-78.67,1,1.56,6,0,8,8,-1,6,2,8,4,2,0,0,0,0,0,
/* 467,fi:0.0002,di:0.40 */-106.66,1,1.56,2,1,12,6,-1,2,3,12,2,3,0,0,0,0,0,
/* 468,fi:0.0002,di:0.40 */53.55,-1,1.56,3,5,2,8,-1,3,7,2,4,2,0,0,0,0,0,
/* 469,fi:0.0002,di:0.40 */127.66,-1,1.56,1,5,14,4,-1,1,7,14,2,2,0,0,0,0,0,
/* 470,fi:0.0002,di:0.40 */-172.69,1,1.56,4,0,6,8,-1,4,2,6,4,2,0,0,0,0,0,
/* 471,fi:0.0002,di:0.40 */119.77,-1,1.56,3,2,2,8,-1,3,6,2,4,2,0,0,0,0,0,
/* 472,fi:0.0002,di:0.40 */-173.00,1,1.56,9,1,4,8,-1,11,1,2,8,2,0,0,0,0,0,
/* 473,fi:0.0002,di:0.40 */127.81,-1,1.56,16,2,2,8,-1,16,6,2,4,2,0,0,0,0,0,
/* 474,fi:0.0002,di:0.40 */164.83,-1,1.56,1,5,12,8,-1,1,7,12,4,2,0,0,0,0,0,
/* 475,fi:0.0002,di:0.40 */561.19,-1,1.56,2,2,12,8,-1,2,6,12,4,2,0,0,0,0,0,
/* 476,fi:0.0002,di:0.40 */-167.63,1,1.56,2,0,6,8,-1,2,2,6,4,2,0,0,0,0,0,
/* 477,fi:0.0002,di:0.40 */148.35,-1,1.56,11,2,6,12,-1,11,6,6,4,3,0,0,0,0,0,
/* 478,fi:0.0002,di:0.39 */242.55,-1,1.56,6,1,4,10,-1,8,1,2,10,2,0,0,0,0,0,
/* 479,fi:0.0002,di:0.39 */-114.42,1,1.56,4,4,6,8,-1,6,4,2,8,3,0,0,0,0,0,
/* 480,fi:0.0002,di:0.39 */495.78,-1,1.56,3,2,8,12,-1,3,6,8,4,3,0,0,0,0,0,
/* 481,fi:0.0002,di:0.39 */417.61,-1,1.56,4,3,10,12,-1,4,7,10,4,3,0,0,0,0,0,
/* 482,fi:0.0002,di:0.39 */100.58,-1,1.56,9,6,8,6,-1,9,8,8,2,3,0,0,0,0,0,
/* 483,fi:0.0002,di:0.39 */330.95,-1,1.55,1,2,6,8,-1,1,6,6,4,2,0,0,0,0,0,
/* 484,fi:0.0002,di:0.39 */28.40,-1,1.55,4,5,2,4,-1,4,7,2,2,2,0,0,0,0,0,
/* 485,fi:0.0002,di:0.39 */525.50,-1,1.55,14,2,4,16,-1,14,6,4,8,2,0,0,0,0,0,
/* 486,fi:0.0002,di:0.39 */114.82,-1,1.55,9,5,8,8,-1,9,7,8,4,2,0,0,0,0,0,
/* 487,fi:0.0002,di:0.39 */159.73,-1,1.55,4,5,14,4,-1,4,7,14,2,2,0,0,0,0,0,
/* 488,fi:0.0002,di:0.39 */-102.39,1,1.55,6,1,10,6,-1,6,3,10,2,3,0,0,0,0,0,
/* 489,fi:0.0002,di:0.39 */162.34,-1,1.55,9,3,6,12,-1,9,7,6,4,3,0,0,0,0,0,
/* 490,fi:0.0002,di:0.39 */-37.50,1,1.55,10,1,8,6,-1,10,3,8,2,3,0,0,0,0,0,
/* 491,fi:0.0002,di:0.39 */-100.74,1,1.55,7,1,10,6,-1,7,3,10,2,3,0,0,0,0,0,
/* 492,fi:0.0002,di:0.39 */-116.76,1,1.55,9,2,4,6,-1,11,2,2,6,2,0,0,0,0,0,
/* 493,fi:0.0002,di:0.39 */93.36,-1,1.55,2,5,4,8,-1,2,7,4,4,2,0,0,0,0,0,
/* 494,fi:0.0002,di:0.39 */25.63,-1,1.55,7,8,6,2,-1,9,8,2,2,3,0,0,0,0,0,
/* 495,fi:0.0002,di:0.39 */246.55,-1,1.55,10,2,6,8,-1,10,6,6,4,2,0,0,0,0,0,
/* 496,fi:0.0002,di:0.38 */-67.12,1,1.55,14,0,2,8,-1,14,2,2,4,2,0,0,0,0,0,
/* 497,fi:0.0002,di:0.38 */-100.38,1,1.55,3,1,8,6,-1,3,3,8,2,3,0,0,0,0,0,
/* 498,fi:0.0002,di:0.38 */-125.23,1,1.55,3,0,4,8,-1,3,2,4,4,2,0,0,0,0,0,
/* 499,fi:0.0002,di:0.38 */138.92,-1,1.55,2,5,12,8,-1,2,7,12,4,2,0,0,0,0,0,
/* 500,fi:0.0002,di:0.38 */388.89,-1,1.55,14,4,4,12,-1,14,8,4,4,3,0,0,0,0,0,
/* 501,fi:0.0002,di:0.38 */139.37,-1,1.55,1,4,8,8,-1,1,6,8,4,2,0,0,0,0,0,
/* 502,fi:0.0002,di:0.38 */202.43,-1,1.55,0,6,14,6,-1,0,8,14,2,3,0,0,0,0,0,
/* 503,fi:0.0002,di:0.38 */435.01,-1,1.55,4,3,12,8,-1,4,7,12,4,2,0,0,0,0,0,
/* 504,fi:0.0002,di:0.38 */151.96,-1,1.55,1,5,18,6,-1,1,7,18,2,3,0,0,0,0,0,
/* 505,fi:0.0002,di:0.38 */155.54,-1,1.55,5,5,12,8,-1,5,7,12,4,2,0,0,0,0,0,
/* 506,fi:0.0002,di:0.38 */-123.30,1,1.55,13,0,4,8,-1,13,2,4,4,2,0,0,0,0,0,
/* 507,fi:0.0002,di:0.38 */664.64,-1,1.55,12,2,6,16,-1,12,6,6,8,2,0,0,0,0,0,
/* 508,fi:0.0002,di:0.38 */147.57,-1,1.55,5,5,14,4,-1,5,7,14,2,2,0,0,0,0,0,
/* 509,fi:0.0002,di:0.38 */-7.94,1,1.54,9,3,4,2,-1,11,3,2,2,2,0,0,0,0,0,
/* 510,fi:0.0002,di:0.38 */-117.41,1,1.61,4,3,6,10,-1,6,3,2,10,3,0,0,0,0,0,
/* 511,fi:0.0002,di:0.38 */-115.26,1,1.60,4,3,6,8,-1,6,3,2,8,3,0,0,0,0,0,
/* 512,fi:0.0002,di:0.37 */-61.33,1,1.59,4,6,6,6,-1,6,6,2,6,3,0,0,0,0,0,
/* 513,fi:0.0002,di:0.37 */-130.80,1,1.58,4,1,6,12,-1,6,1,2,12,3,0,0,0,0,0,
/* 514,fi:0.0002,di:0.37 */-84.69,1,1.57,4,5,6,6,-1,6,5,2,6,3,0,0,0,0,0,
/* 515,fi:0.0002,di:0.36 */-35.23,1,1.56,4,5,6,8,-1,6,5,2,8,3,0,0,0,0,0,
/* 516,fi:0.0002,di:0.36 */-323.21,1,1.55,2,1,8,16,-1,4,1,4,16,2,0,0,0,0,0,
/* 517,fi:0.0002,di:0.36 */67.93,1,1.55,4,2,14,6,-1,4,4,14,2,3,0,0,0,0,0,
/* 518,fi:0.0002,di:0.36 */136.10,-1,1.55,5,6,10,6,-1,5,8,10,2,3,0,0,0,0,0,
/* 519,fi:0.0002,di:0.36 */-93.09,1,1.55,4,2,6,12,-1,6,2,2,12,3,0,0,0,0,0,
/* 520,fi:0.0002,di:0.36 */-34.57,1,1.55,0,1,12,6,-1,0,3,12,2,3,0,0,0,0,0,
/* 521,fi:0.0002,di:0.36 */64.07,-1,1.54,3,4,4,8,-1,3,6,4,4,2,0,0,0,0,0,
/* 522,fi:0.0002,di:0.36 */150.84,-1,1.54,6,6,10,6,-1,6,8,10,2,3,0,0,0,0,0,
/* 523,fi:0.0002,di:0.36 */-35.19,1,1.54,1,1,10,6,-1,1,3,10,2,3,0,0,0,0,0,
/* 524,fi:0.0002,di:0.36 */32.60,-1,1.54,7,7,6,2,-1,9,7,2,2,3,0,0,0,0,0,
/* 525,fi:0.0002,di:0.36 */109.66,-1,1.54,4,4,8,4,-1,4,6,8,2,2,0,0,0,0,0,
/* 526,fi:0.0002,di:0.36 */104.78,-1,1.54,9,5,10,4,-1,9,7,10,2,2,0,0,0,0,0,
/* 527,fi:0.0002,di:0.36 */134.77,-1,1.54,2,6,8,6,-1,2,8,8,2,3,0,0,0,0,0,
/* 528,fi:0.0002,di:0.36 */-122.92,1,1.54,4,1,6,10,-1,6,1,2,10,3,0,0,0,0,0,
/* 529,fi:0.0002,di:0.36 */50.09,-1,1.54,8,4,6,8,-1,8,6,6,4,2,0,0,0,0,0,
/* 530,fi:0.0002,di:0.36 */-57.11,1,1.54,2,1,8,6,-1,2,3,8,2,3,0,0,0,0,0,
/* 531,fi:0.0002,di:0.36 */-221.38,1,1.54,0,0,10,8,-1,0,2,10,4,2,0,0,0,0,0,
/* 532,fi:0.0002,di:0.36 */40.01,-1,1.53,4,4,2,8,-1,4,6,2,4,2,0,0,0,0,0,
/* 533,fi:0.0002,di:0.36 */-69.43,1,1.53,9,1,10,6,-1,9,3,10,2,3,0,0,0,0,0,
/* 534,fi:0.0002,di:0.36 */354.77,-1,1.53,4,2,8,12,-1,4,6,8,4,3,0,0,0,0,0,
/* 535,fi:0.0002,di:0.36 */-87.10,1,1.53,0,1,14,6,-1,0,3,14,2,3,0,0,0,0,0,
/* 536,fi:0.0002,di:0.36 */35.58,-1,1.53,2,4,4,4,-1,2,6,4,2,2,0,0,0,0,0,
/* 537,fi:0.0002,di:0.36 */53.75,-1,1.53,7,6,6,2,-1,9,6,2,2,3,0,0,0,0,0,
/* 538,fi:0.0002,di:0.35 */-41.34,1,1.53,4,7,6,4,-1,6,7,2,4,3,0,0,0,0,0,
/* 539,fi:0.0002,di:0.35 */-104.48,1,1.53,5,0,4,8,-1,5,2,4,4,2,0,0,0,0,0,
/* 540,fi:0.0002,di:0.35 */111.43,-1,1.53,8,5,10,4,-1,8,7,10,2,2,0,0,0,0,0,
/* 541,fi:0.0002,di:0.35 */130.75,-1,1.53,9,6,10,6,-1,9,8,10,2,3,0,0,0,0,0,
/* 542,fi:0.0002,di:0.35 */35.80,-1,1.53,3,5,12,6,-1,3,7,12,2,3,0,0,0,0,0,
/* 543,fi:0.0002,di:0.35 */85.90,-1,1.53,11,5,8,4,-1,11,7,8,2,2,0,0,0,0,0,
/* 544,fi:0.0002,di:0.35 */185.89,-1,1.53,6,2,4,8,-1,8,2,2,8,2,0,0,0,0,0,
/* 545,fi:0.0002,di:0.35 */-214.79,1,1.53,2,1,8,10,-1,4,1,4,10,2,0,0,0,0,0,
/* 546,fi:0.0002,di:0.35 */125.91,-1,1.53,7,5,10,8,-1,7,7,10,4,2,0,0,0,0,0,
/* 547,fi:0.0002,di:0.35 */62.06,-1,1.53,4,4,4,4,-1,4,6,4,2,2,0,0,0,0,0,
/* 548,fi:0.0002,di:0.35 */500.25,-1,1.53,0,2,10,12,-1,0,6,10,4,3,0,0,0,0,0,
/* 549,fi:0.0002,di:0.35 */33.47,1,1.52,1,1,8,6,-1,1,3,8,2,3,0,0,0,0,0,
/* 550,fi:0.0002,di:0.35 */130.36,-1,1.52,7,5,12,4,-1,7,7,12,2,2,0,0,0,0,0,
/* 551,fi:0.0002,di:0.35 */133.85,-1,1.52,1,5,8,8,-1,1,7,8,4,2,0,0,0,0,0,
/* 552,fi:0.0002,di:0.35 */97.39,-1,1.52,5,6,6,6,-1,5,8,6,2,3,0,0,0,0,0,
/* 553,fi:0.0002,di:0.35 */437.31,-1,1.52,2,2,8,12,-1,2,6,8,4,3,0,0,0,0,0,
/* 554,fi:0.0002,di:0.35 */114.84,-1,1.52,5,6,8,6,-1,5,8,8,2,3,0,0,0,0,0,
/* 555,fi:0.0002,di:0.35 */38.38,1,1.52,2,2,14,6,-1,2,4,14,2,3,0,0,0,0,0,
/* 556,fi:0.0002,di:0.35 */130.38,-1,1.52,10,5,8,8,-1,10,7,8,4,2,0,0,0,0,0,
/* 557,fi:0.0002,di:0.35 */62.34,-1,1.52,15,4,4,4,-1,15,6,4,2,2,0,0,0,0,0,
/* 558,fi:0.0002,di:0.35 */-79.46,1,1.52,4,2,6,16,-1,6,2,2,16,3,0,0,0,0,0,
/* 559,fi:0.0002,di:0.35 */45.12,1,1.52,1,2,8,6,-1,1,4,8,2,3,0,0,0,0,0,
/* 560,fi:0.0002,di:0.35 */72.02,-1,1.52,4,5,8,8,-1,4,7,8,4,2,0,0,0,0,0,
/* 561,fi:0.0002,di:0.35 */58.68,-1,1.52,11,5,6,4,-1,11,7,6,2,2,0,0,0,0,0,
/* 562,fi:0.0002,di:0.34 */-187.56,1,1.52,1,1,18,8,-1,1,3,18,4,2,0,0,0,0,0,
/* 563,fi:0.0002,di:0.34 */130.66,-1,1.52,14,5,4,8,-1,14,7,4,4,2,0,0,0,0,0,
/* 564,fi:0.0002,di:0.34 */97.66,-1,1.52,3,5,10,8,-1,3,7,10,4,2,0,0,0,0,0,
/* 565,fi:0.0002,di:0.34 */67.09,-1,1.52,15,5,2,8,-1,15,7,2,4,2,0,0,0,0,0,
/* 566,fi:0.0002,di:0.34 */561.57,-1,1.52,0,2,12,8,-1,0,6,12,4,2,0,0,0,0,0,
/* 567,fi:0.0002,di:0.34 */22.94,-1,1.52,13,4,2,4,-1,13,6,2,2,2,0,0,0,0,0,
/* 568,fi:0.0002,di:0.34 */77.38,-1,1.52,13,5,6,4,-1,13,7,6,2,2,0,0,0,0,0,
/* 569,fi:0.0002,di:0.34 */-218.92,1,1.52,2,1,16,8,-1,2,3,16,4,2,0,0,0,0,0,
/* 570,fi:0.0002,di:0.34 */58.53,-1,1.52,1,5,6,4,-1,1,7,6,2,2,0,0,0,0,0,
/* 571,fi:0.0002,di:0.34 */99.39,-1,1.51,4,6,6,6,-1,4,8,6,2,3,0,0,0,0,0,
/* 572,fi:0.0002,di:0.34 */321.95,-1,1.51,0,2,6,8,-1,0,6,6,4,2,0,0,0,0,0,
/* 573,fi:0.0002,di:0.34 */-162.09,1,1.51,0,1,18,8,-1,0,3,18,4,2,0,0,0,0,0,
/* 574,fi:0.0002,di:0.34 */-195.74,1,1.51,9,2,8,10,-1,11,2,4,10,2,0,0,0,0,0,
/* 575,fi:0.0002,di:0.34 */-147.18,1,1.51,13,0,6,8,-1,13,2,6,4,2,0,0,0,0,0,
/* 576,fi:0.0002,di:0.34 */192.38,-1,1.51,6,5,12,8,-1,6,7,12,4,2,0,0,0,0,0,
/* 577,fi:0.0002,di:0.34 */59.28,-1,1.51,6,4,6,8,-1,6,6,6,4,2,0,0,0,0,0,
/* 578,fi:0.0002,di:0.34 */381.13,-1,1.51,0,2,8,8,-1,0,6,8,4,2,0,0,0,0,0,
/* 579,fi:0.0002,di:0.34 */-169.85,1,1.51,2,2,8,10,-1,4,2,4,10,2,0,0,0,0,0,
/* 580,fi:0.0002,di:0.34 */-289.95,1,1.51,2,0,8,16,-1,4,0,4,16,2,0,0,0,0,0,
/* 581,fi:0.0002,di:0.34 */30.44,-1,1.51,3,5,2,4,-1,3,7,2,2,2,0,0,0,0,0,
/* 582,fi:0.0002,di:0.34 */71.24,-1,1.51,9,5,8,4,-1,9,7,8,2,2,0,0,0,0,0,
/* 583,fi:0.0002,di:0.34 */54.43,1,1.51,3,2,14,6,-1,3,4,14,2,3,0,0,0,0,0,
/* 584,fi:0.0002,di:0.34 */67.46,-1,1.51,14,5,4,4,-1,14,7,4,2,2,0,0,0,0,0,
/* 585,fi:0.0002,di:0.34 */-101.93,1,1.51,4,1,10,6,-1,4,3,10,2,3,0,0,0,0,0,
/* 586,fi:0.0002,di:0.34 */373.38,-1,1.51,1,2,8,8,-1,1,6,8,4,2,0,0,0,0,0,
/* 587,fi:0.0002,di:0.34 */112.06,-1,1.51,1,5,16,6,-1,1,7,16,2,3,0,0,0,0,0,
/* 588,fi:0.0002,di:0.34 */51.84,-1,1.51,1,4,6,8,-1,1,6,6,4,2,0,0,0,0,0,
/* 589,fi:0.0002,di:0.34 */-94.36,1,1.51,12,0,4,8,-1,12,2,4,4,2,0,0,0,0,0,
/* 590,fi:0.0002,di:0.34 */78.48,-1,1.51,4,4,4,8,-1,4,6,4,4,2,0,0,0,0,0,
/* 591,fi:0.0002,di:0.34 */44.25,-1,1.51,4,5,12,6,-1,4,7,12,2,3,0,0,0,0,0,
/* 592,fi:0.0002,di:0.34 */83.03,-1,1.51,2,4,6,8,-1,2,6,6,4,2,0,0,0,0,0,
/* 593,fi:0.0002,di:0.34 */-32.85,1,1.51,3,1,2,8,-1,3,3,2,4,2,0,0,0,0,0,
/* 594,fi:0.0002,di:0.34 */120.87,-1,1.51,5,2,8,12,-1,5,6,8,4,3,0,0,0,0,0,
/* 595,fi:0.0002,di:0.34 */200.76,-1,1.51,6,1,4,8,-1,8,1,2,8,2,0,0,0,0,0,
/* 596,fi:0.0002,di:0.34 */376.46,-1,1.51,5,3,10,12,-1,5,7,10,4,3,0,0,0,0,0,
/* 597,fi:0.0002,di:0.34 */86.36,-1,1.51,7,2,8,12,-1,7,6,8,4,3,0,0,0,0,0,
/* 598,fi:0.0002,di:0.34 */-89.82,1,1.51,4,1,6,16,-1,6,1,2,16,3,0,0,0,0,0,
/* 599,fi:0.0002,di:0.34 */543.49,-1,1.50,1,2,12,8,-1,1,6,12,4,2,0,0,0,0,0,
/* 600,fi:0.0002,di:0.34 */-35.74,1,1.50,5,0,2,8,-1,5,2,2,4,2,0,0,0,0,0,
/* 601,fi:0.0002,di:0.34 */60.87,-1,1.50,12,5,6,4,-1,12,7,6,2,2,0,0,0,0,0,
/* 602,fi:0.0002,di:0.34 */-202.37,1,1.50,2,3,8,14,-1,4,3,4,14,2,0,0,0,0,0,
/* 603,fi:0.0002,di:0.34 */373.05,-1,1.50,4,3,8,12,-1,4,7,8,4,3,0,0,0,0,0,
/* 604,fi:0.0002,di:0.34 */-155.33,1,1.50,2,2,8,8,-1,4,2,4,8,2,0,0,0,0,0,
/* 605,fi:0.0002,di:0.34 */339.62,-1,1.50,4,2,6,12,-1,4,6,6,4,3,0,0,0,0,0,
/* 606,fi:0.0002,di:0.34 */91.45,-1,1.50,1,5,8,4,-1,1,7,8,2,2,0,0,0,0,0,
/* 607,fi:0.0002,di:0.34 */37.73,1,1.50,0,2,16,6,-1,0,4,16,2,3,0,0,0,0,0,
/* 608,fi:0.0002,di:0.34 */-51.20,1,1.50,4,2,6,14,-1,6,2,2,14,3,0,0,0,0,0,
/* 609,fi:0.0002,di:0.34 */421.18,-1,1.50,7,2,10,8,-1,7,6,10,4,2,0,0,0,0,0,
/* 610,fi:0.0002,di:0.34 */-57.97,1,1.50,4,4,6,6,-1,6,4,2,6,3,0,0,0,0,0,
/* 611,fi:0.0002,di:0.34 */126.41,-1,1.50,15,5,4,8,-1,15,7,4,4,2,0,0,0,0,0,
/* 612,fi:0.0002,di:0.34 */474.57,-1,1.50,1,2,10,8,-1,1,6,10,4,2,0,0,0,0,0,
/* 613,fi:0.0002,di:0.34 */21.33,-1,1.50,9,4,6,4,-1,9,6,6,2,2,0,0,0,0,0,
/* 614,fi:0.0002,di:0.33 */180.07,-1,1.50,17,3,2,12,-1,17,7,2,4,3,0,0,0,0,0,
/* 615,fi:0.0002,di:0.33 */302.25,-1,1.50,13,4,4,12,-1,13,8,4,4,3,0,0,0,0,0,
/* 616,fi:0.0002,di:0.33 */799.10,-1,1.50,11,2,8,16,-1,11,6,8,8,2,0,0,0,0,0,
/* 617,fi:0.0002,di:0.33 */127.65,-1,1.50,3,5,16,6,-1,3,7,16,2,3,0,0,0,0,0,
/* 618,fi:0.0002,di:0.33 */-95.83,1,1.50,1,0,6,8,-1,1,2,6,4,2,0,0,0,0,0,
/* 619,fi:0.0002,di:0.33 */39.30,-1,1.50,5,4,2,8,-1,5,6,2,4,2,0,0,0,0,0,
/* 620,fi:0.0002,di:0.33 */-183.57,1,1.50,3,1,16,8,-1,3,3,16,4,2,0,0,0,0,0,
/* 621,fi:0.0002,di:0.33 */-181.36,1,1.50,1,1,16,8,-1,1,3,16,4,2,0,0,0,0,0,
/* 622,fi:0.0002,di:0.33 */54.98,-1,1.50,12,4,6,8,-1,12,6,6,4,2,0,0,0,0,0,
/* 623,fi:0.0002,di:0.32 */-12.13,1,1.50,4,8,6,2,-1,6,8,2,2,3,0,0,0,0,0,
/* 624,fi:0.0002,di:0.32 */-118.22,1,1.51,4,0,6,12,-1,6,0,2,12,3,0,0,0,0,0,
/* 625,fi:0.0002,di:0.32 */-36.76,1,1.51,4,6,6,4,-1,6,6,2,4,3,0,0,0,0,0,
/* 626,fi:0.0002,di:0.32 */-259.38,1,1.50,2,1,8,14,-1,4,1,4,14,2,0,0,0,0,0,
/* 627,fi:0.0002,di:0.32 */-117.73,1,1.50,2,3,8,8,-1,4,3,4,8,2,0,0,0,0,0,
/* 628,fi:0.0002,di:0.32 */120.06,-1,1.50,6,5,12,4,-1,6,7,12,2,2,0,0,0,0,0,
/* 629,fi:0.0002,di:0.32 */105.59,1,1.50,5,2,14,6,-1,5,4,14,2,3,0,0,0,0,0,
/* 630,fi:0.0002,di:0.32 */-66.97,1,1.50,4,3,6,14,-1,6,3,2,14,3,0,0,0,0,0,
/* 631,fi:0.0002,di:0.32 */-65.30,1,1.50,2,0,4,8,-1,2,2,4,4,2,0,0,0,0,0,
/* 632,fi:0.0002,di:0.32 */89.75,-1,1.50,4,2,2,12,-1,4,6,2,4,3,0,0,0,0,0,
/* 633,fi:0.0002,di:0.32 */-17.12,1,1.50,4,4,6,10,-1,6,4,2,10,3,0,0,0,0,0,
/* 634,fi:0.0002,di:0.32 */-76.13,1,1.50,4,2,6,8,-1,6,2,2,8,3,0,0,0,0,0,
/* 635,fi:0.0002,di:0.32 */484.44,-1,1.50,0,2,10,8,-1,0,6,10,4,2,0,0,0,0,0,
/* 636,fi:0.0002,di:0.32 */68.71,-1,1.50,3,5,14,6,-1,3,7,14,2,3,0,0,0,0,0,
/* 637,fi:0.0002,di:0.32 */28.70,-1,1.50,11,4,4,4,-1,11,6,4,2,2,0,0,0,0,0,
/* 638,fi:0.0002,di:0.32 */97.13,-1,1.50,4,5,12,4,-1,4,7,12,2,2,0,0,0,0,0,
/* 639,fi:0.0002,di:0.32 */61.29,-1,1.50,2,5,14,6,-1,2,7,14,2,3,0,0,0,0,0,
/* 640,fi:0.0002,di:0.32 */146.45,-1,1.50,0,5,18,6,-1,0,7,18,2,3,0,0,0,0,0,
/* 641,fi:0.0002,di:0.32 */131.83,-1,1.50,7,1,6,4,-1,9,1,2,4,3,0,0,0,0,0,
/* 642,fi:0.0002,di:0.32 */-131.19,1,1.50,0,0,8,8,-1,0,2,8,4,2,0,0,0,0,0,
/* 643,fi:0.0002,di:0.32 */-65.32,1,1.50,2,2,8,4,-1,4,2,4,4,2,0,0,0,0,0,
/* 644,fi:0.0002,di:0.32 */-42.18,1,1.49,4,1,4,6,-1,4,3,4,2,3,0,0,0,0,0,
/* 645,fi:0.0002,di:0.32 */-188.93,1,1.49,2,2,8,12,-1,4,2,4,12,2,0,0,0,0,0,
/* 646,fi:0.0002,di:0.32 */668.84,-1,1.49,14,0,4,18,-1,14,6,4,6,3,0,0,0,0,0,
/* 647,fi:0.0002,di:0.32 */-63.50,1,1.49,9,3,4,4,-1,11,3,2,4,2,0,0,0,0,0,
/* 648,fi:0.0002,di:0.32 */-110.03,1,1.49,9,3,4,6,-1,11,3,2,6,2,0,0,0,0,0,
/* 649,fi:0.0002,di:0.32 */-221.32,1,1.49,2,1,8,12,-1,4,1,4,12,2,0,0,0,0,0,
/* 650,fi:0.0002,di:0.32 */75.08,-1,1.49,0,4,6,4,-1,0,6,6,2,2,0,0,0,0,0,
/* 651,fi:0.0002,di:0.32 */57.36,1,1.49,1,2,14,6,-1,1,4,14,2,3,0,0,0,0,0,
/* 652,fi:0.0002,di:0.32 */194.93,-1,1.49,15,4,2,12,-1,15,8,2,4,3,0,0,0,0,0,
/* 653,fi:0.0002,di:0.32 */-44.44,1,1.49,10,1,6,6,-1,10,3,6,2,3,0,0,0,0,0,
/* 654,fi:0.0002,di:0.32 */16.61,1,1.49,0,1,10,6,-1,0,3,10,2,3,0,0,0,0,0,
/* 655,fi:0.0002,di:0.32 */90.12,-1,1.49,4,5,14,6,-1,4,7,14,2,3,0,0,0,0,0,
/* 656,fi:0.0002,di:0.32 */47.64,-1,1.49,1,5,4,4,-1,1,7,4,2,2,0,0,0,0,0,
/* 657,fi:0.0002,di:0.32 */13.64,-1,1.49,1,4,4,4,-1,1,6,4,2,2,0,0,0,0,0,
/* 658,fi:0.0002,di:0.32 */-137.54,1,1.49,9,0,8,10,-1,11,0,4,10,2,0,0,0,0,0,
/* 659,fi:0.0002,di:0.32 */-108.22,1,1.49,9,1,8,8,-1,11,1,4,8,2,0,0,0,0,0,
/* 660,fi:0.0002,di:0.32 */-106.65,1,1.49,2,1,8,6,-1,4,1,4,6,2,0,0,0,0,0,
/* 661,fi:0.0002,di:0.32 */84.43,-1,1.49,5,4,4,8,-1,5,6,4,4,2,0,0,0,0,0,
/* 662,fi:0.0002,di:0.32 */-65.05,1,1.49,9,1,8,6,-1,9,3,8,2,3,0,0,0,0,0,
/* 663,fi:0.0002,di:0.32 */94.85,-1,1.49,3,5,12,4,-1,3,7,12,2,2,0,0,0,0,0,
/* 664,fi:0.0002,di:0.32 */54.19,-1,1.49,13,5,4,4,-1,13,7,4,2,2,0,0,0,0,0,
/* 665,fi:0.0002,di:0.31 */-265.69,1,1.49,2,2,8,16,-1,4,2,4,16,2,0,0,0,0,0,
/* 666,fi:0.0002,di:0.31 */386.23,-1,1.49,16,1,2,18,-1,16,7,2,6,3,0,0,0,0,0,
/* 667,fi:0.0002,di:0.31 */109.36,-1,1.49,2,5,16,6,-1,2,7,16,2,3,0,0,0,0,0,
/* 668,fi:0.0002,di:0.31 */17.74,-1,1.49,10,4,4,8,-1,10,6,4,4,2,0,0,0,0,0,
/* 669,fi:0.0002,di:0.31 */33.57,-1,1.49,5,4,8,4,-1,5,6,8,2,2,0,0,0,0,0,
/* 670,fi:0.0002,di:0.31 */102.62,-1,1.49,2,5,12,4,-1,2,7,12,2,2,0,0,0,0,0,
/* 671,fi:0.0002,di:0.31 */108.41,-1,1.49,1,5,10,4,-1,1,7,10,2,2,0,0,0,0,0,
/* 672,fi:0.0002,di:0.31 */1433.06,-1,1.49,0,2,18,16,-1,0,6,18,8,2,0,0,0,0,0,
/* 673,fi:0.0002,di:0.31 */107.25,-1,1.49,1,5,6,8,-1,1,7,6,4,2,0,0,0,0,0,
/* 674,fi:0.0002,di:0.31 */-86.69,1,1.49,4,1,6,6,-1,4,3,6,2,3,0,0,0,0,0,
/* 675,fi:0.0002,di:0.31 */-96.51,1,1.49,14,0,4,8,-1,14,2,4,4,2,0,0,0,0,0,
/* 676,fi:0.0002,di:0.31 */453.99,-1,1.49,6,3,12,8,-1,6,7,12,4,2,0,0,0,0,0,
/* 677,fi:0.0002,di:0.31 */-33.27,1,1.49,10,0,4,8,-1,10,2,4,4,2,0,0,0,0,0,
/* 678,fi:0.0002,di:0.31 */22.53,-1,1.48,15,4,2,4,-1,15,6,2,2,2,0,0,0,0,0,
/* 679,fi:0.0002,di:0.31 */97.33,-1,1.48,5,5,14,6,-1,5,7,14,2,3,0,0,0,0,0,
/* 680,fi:0.0002,di:0.31 */-44.53,1,1.48,4,1,6,14,-1,6,1,2,14,3,0,0,0,0,0,
/* 681,fi:0.0002,di:0.31 */108.47,-1,1.48,7,6,8,6,-1,7,8,8,2,3,0,0,0,0,0,
/* 682,fi:0.0002,di:0.31 */-184.15,1,1.48,3,1,14,8,-1,3,3,14,4,2,0,0,0,0,0,
/* 683,fi:0.0002,di:0.31 */85.53,-1,1.48,1,5,4,8,-1,1,7,4,4,2,0,0,0,0,0,
/* 684,fi:0.0002,di:0.31 */123.60,-1,1.48,6,2,4,6,-1,8,2,2,6,2,0,0,0,0,0,
/* 685,fi:0.0002,di:0.31 */140.29,-1,1.48,3,2,4,12,-1,3,6,4,4,3,0,0,0,0,0,
/* 686,fi:0.0002,di:0.31 */30.80,-1,1.48,12,4,4,8,-1,12,6,4,4,2,0,0,0,0,0,
/* 687,fi:0.0002,di:0.31 */113.41,-1,1.48,0,5,14,4,-1,0,7,14,2,2,0,0,0,0,0,
/* 688,fi:0.0002,di:0.31 */63.22,-1,1.48,13,4,6,8,-1,13,6,6,4,2,0,0,0,0,0,
/* 689,fi:0.0002,di:0.31 */37.43,1,1.48,2,1,6,6,-1,2,3,6,2,3,0,0,0,0,0,
/* 690,fi:0.0002,di:0.31 */41.62,-1,1.48,3,5,4,4,-1,3,7,4,2,2,0,0,0,0,0,
/* 691,fi:0.0002,di:0.31 */-21.26,1,1.48,4,3,6,12,-1,6,3,2,12,3,0,0,0,0,0,
/* 692,fi:0.0002,di:0.31 */86.87,-1,1.48,1,5,14,6,-1,1,7,14,2,3,0,0,0,0,0,
/* 693,fi:0.0002,di:0.31 */94.39,-1,1.48,2,5,6,8,-1,2,7,6,4,2,0,0,0,0,0,
/* 694,fi:0.0002,di:0.31 */116.97,-1,1.48,0,5,12,4,-1,0,7,12,2,2,0,0,0,0,0,
/* 695,fi:0.0002,di:0.31 */61.62,-1,1.48,8,5,8,8,-1,8,7,8,4,2,0,0,0,0,0,
/* 696,fi:0.0002,di:0.31 */132.82,-1,1.48,11,5,8,8,-1,11,7,8,4,2,0,0,0,0,0,
/* 697,fi:0.0002,di:0.31 */106.60,-1,1.48,1,5,12,4,-1,1,7,12,2,2,0,0,0,0,0,
/* 698,fi:0.0002,di:0.31 */84.06,-1,1.48,13,3,2,8,-1,13,7,2,4,2,0,0,0,0,0,
/* 699,fi:0.0002,di:0.31 */-32.56,1,1.48,3,0,2,8,-1,3,2,2,4,2,0,0,0,0,0,
/* 700,fi:0.0002,di:0.31 */72.48,-1,1.48,9,6,6,6,-1,9,8,6,2,3,0,0,0,0,0,
/* 701,fi:0.0002,di:0.31 */68.91,-1,1.48,0,5,6,4,-1,0,7,6,2,2,0,0,0,0,0,
/* 702,fi:0.0002,di:0.31 */375.94,-1,1.48,1,3,10,8,-1,1,7,10,4,2,0,0,0,0,0,
/* 703,fi:0.0002,di:0.31 */287.38,-1,1.48,2,2,6,8,-1,2,6,6,4,2,0,0,0,0,0,
/* 704,fi:0.0002,di:0.31 */187.87,-1,1.48,10,3,6,8,-1,10,7,6,4,2,0,0,0,0,0,
/* 705,fi:0.0002,di:0.31 */-86.99,1,1.48,8,1,8,6,-1,8,3,8,2,3,0,0,0,0,0,
/* 706,fi:0.0002,di:0.31 */104.77,-1,1.48,5,5,12,4,-1,5,7,12,2,2,0,0,0,0,0,
/* 707,fi:0.0002,di:0.31 */62.80,-1,1.48,5,5,12,6,-1,5,7,12,2,3,0,0,0,0,0,
/* 708,fi:0.0002,di:0.31 */153.44,-1,1.48,6,1,4,6,-1,8,1,2,6,2,0,0,0,0,0,
/* 709,fi:0.0002,di:0.30 */-1.50,1,1.48,4,9,6,2,-1,6,9,2,2,3,0,0,0,0,0,
/* 710,fi:0.0002,di:0.30 */30.86,-1,1.48,7,4,8,4,-1,7,6,8,2,2,0,0,0,0,0,
/* 711,fi:0.0002,di:0.30 */9.15,1,1.48,9,9,2,4,-1,9,11,2,2,2,0,0,0,0,0,
/* 712,fi:0.0002,di:0.30 */71.78,-1,1.50,8,6,6,6,-1,8,8,6,2,3,0,0,0,0,0,
/* 713,fi:0.0002,di:0.30 */9.77,-1,1.49,7,4,6,8,-1,7,6,6,4,2,0,0,0,0,0,
/* 714,fi:0.0002,di:0.30 */-290.78,1,1.49,2,0,8,18,-1,4,0,4,18,2,0,0,0,0,0,
/* 715,fi:0.0002,di:0.30 */46.74,-1,1.49,2,5,12,6,-1,2,7,12,2,3,0,0,0,0,0,
/* 716,fi:0.0002,di:0.30 */-10.34,1,1.49,9,4,4,2,-1,11,4,2,2,2,0,0,0,0,0,
/* 717,fi:0.0002,di:0.30 */-123.07,1,1.48,2,3,8,12,-1,4,3,4,12,2,0,0,0,0,0,
/* 718,fi:0.0002,di:0.30 */104.40,-1,1.48,6,6,8,6,-1,6,8,8,2,3,0,0,0,0,0,
/* 719,fi:0.0002,di:0.30 */-23.43,-1,1.48,7,3,4,6,-1,9,3,2,6,2,0,0,0,0,0,
/* 720,fi:0.0002,di:0.30 */-150.91,1,1.48,2,1,8,8,-1,4,1,4,8,2,0,0,0,0,0,
/* 721,fi:0.0002,di:0.30 */-164.78,1,1.48,9,0,4,8,-1,11,0,2,8,2,0,0,0,0,0,
/* 722,fi:0.0002,di:0.30 */81.99,1,1.48,6,2,12,6,-1,6,4,12,2,3,0,0,0,0,0,
/* 723,fi:0.0002,di:0.30 */-55.27,1,1.48,4,1,6,18,-1,6,1,2,18,3,0,0,0,0,0,
/* 724,fi:0.0002,di:0.30 */14.77,-1,1.48,5,5,10,6,-1,5,7,10,2,3,0,0,0,0,0,
/* 725,fi:0.0002,di:0.30 */135.77,-1,1.48,6,3,4,8,-1,8,3,2,8,2,0,0,0,0,0,
/* 726,fi:0.0002,di:0.30 */109.45,1,1.48,1,2,6,6,-1,1,4,6,2,3,0,0,0,0,0,
/* 727,fi:0.0002,di:0.30 */296.85,-1,1.47,6,1,8,10,-1,8,1,4,10,2,0,0,0,0,0,
/* 728,fi:0.0002,di:0.30 */105.32,-1,1.47,2,5,10,4,-1,2,7,10,2,2,0,0,0,0,0,
/* 729,fi:0.0002,di:0.30 */102.43,-1,1.47,0,4,8,8,-1,0,6,8,4,2,0,0,0,0,0,
/* 730,fi:0.0002,di:0.30 */15.35,-1,1.47,4,5,10,6,-1,4,7,10,2,3,0,0,0,0,0,
/* 731,fi:0.0002,di:0.30 */-170.98,1,1.47,4,1,14,8,-1,4,3,14,4,2,0,0,0,0,0,
/* 732,fi:0.0002,di:0.30 */-51.31,1,1.47,2,1,4,8,-1,2,3,4,4,2,0,0,0,0,0,
/* 733,fi:0.0002,di:0.30 */-73.25,-1,1.47,7,3,4,8,-1,9,3,2,8,2,0,0,0,0,0,
/* 734,fi:0.0002,di:0.30 */52.59,-1,1.47,7,6,6,8,-1,9,6,2,8,3,0,0,0,0,0,
/* 735,fi:0.0002,di:0.29 */-210.26,1,1.47,2,0,8,12,-1,4,0,4,12,2,0,0,0,0,0,
/* 736,fi:0.0002,di:0.29 */-269.07,1,1.47,2,1,8,18,-1,4,1,4,18,2,0,0,0,0,0,
/* 737,fi:0.0002,di:0.29 */-43.45,-1,1.47,7,2,4,8,-1,9,2,2,8,2,0,0,0,0,0,
/* 738,fi:0.0002,di:0.29 */20.57,1,1.47,11,1,8,6,-1,11,3,8,2,3,0,0,0,0,0,
/* 739,fi:0.0002,di:0.29 */105.43,-1,1.47,0,5,16,6,-1,0,7,16,2,3,0,0,0,0,0,
/* 740,fi:0.0002,di:0.29 */-183.11,1,1.47,9,0,8,12,-1,11,0,4,12,2,0,0,0,0,0,
/* 741,fi:0.0002,di:0.29 */60.12,-1,1.47,2,5,6,4,-1,2,7,6,2,2,0,0,0,0,0,
/* 742,fi:0.0002,di:0.29 */-54.66,-1,1.47,7,4,4,6,-1,9,4,2,6,2,0,0,0,0,0,
/* 743,fi:0.0002,di:0.29 */-194.50,1,1.47,9,1,8,12,-1,11,1,4,12,2,0,0,0,0,0,
/* 744,fi:0.0002,di:0.29 */257.09,-1,1.47,6,0,4,12,-1,8,0,2,12,2,0,0,0,0,0,
/* 745,fi:0.0002,di:0.29 */82.14,-1,1.47,0,5,8,4,-1,0,7,8,2,2,0,0,0,0,0,
/* 746,fi:0.0002,di:0.29 */-97.25,1,1.47,4,1,8,6,-1,4,3,8,2,3,0,0,0,0,0,
/* 747,fi:0.0002,di:0.29 */43.11,-1,1.47,10,5,6,4,-1,10,7,6,2,2,0,0,0,0,0,
/* 748,fi:0.0002,di:0.29 */-69.17,1,1.47,4,0,6,14,-1,6,0,2,14,3,0,0,0,0,0,
/* 749,fi:0.0002,di:0.29 */-76.25,1,1.47,2,2,8,6,-1,4,2,4,6,2,0,0,0,0,0,
/* 750,fi:0.0002,di:0.29 */-77.15,1,1.47,4,0,6,18,-1,6,0,2,18,3,0,0,0,0,0,
/* 751,fi:0.0002,di:0.29 */-103.82,1,1.47,5,0,6,8,-1,5,2,6,4,2,0,0,0,0,0,
/* 752,fi:0.0002,di:0.29 */425.74,-1,1.47,15,1,2,18,-1,15,7,2,6,3,0,0,0,0,0,
/* 753,fi:0.0002,di:0.29 */91.34,-1,1.47,7,5,10,4,-1,7,7,10,2,2,0,0,0,0,0,
/* 754,fi:0.0002,di:0.29 */96.74,-1,1.47,3,5,6,8,-1,3,7,6,4,2,0,0,0,0,0,
/* 755,fi:0.0002,di:0.29 */158.46,-1,1.47,13,5,6,8,-1,13,7,6,4,2,0,0,0,0,0,
/* 756,fi:0.0002,di:0.29 */2.87,1,1.47,11,0,2,8,-1,11,2,2,4,2,0,0,0,0,0,
/* 757,fi:0.0002,di:0.29 */385.37,-1,1.47,3,3,12,8,-1,3,7,12,4,2,0,0,0,0,0,
/* 758,fi:0.0002,di:0.29 */82.53,-1,1.47,4,5,6,8,-1,4,7,6,4,2,0,0,0,0,0,
/* 759,fi:0.0002,di:0.29 */185.30,-1,1.47,6,2,4,10,-1,8,2,2,10,2,0,0,0,0,0,
/* 760,fi:0.0002,di:0.29 */363.67,-1,1.47,1,3,12,8,-1,1,7,12,4,2,0,0,0,0,0,
/* 761,fi:0.0002,di:0.29 */185.50,-1,1.47,6,2,8,8,-1,8,2,4,8,2,0,0,0,0,0,
/* 762,fi:0.0002,di:0.29 */-63.90,1,1.47,14,1,4,8,-1,14,3,4,4,2,0,0,0,0,0,
/* 763,fi:0.0002,di:0.29 */118.15,-1,1.47,0,5,6,8,-1,0,7,6,4,2,0,0,0,0,0,
/* 764,fi:0.0002,di:0.29 */-47.39,1,1.46,15,0,2,8,-1,15,2,2,4,2,0,0,0,0,0,
/* 765,fi:0.0002,di:0.29 */-97.33,1,1.46,9,0,6,8,-1,9,2,6,4,2,0,0,0,0,0,
/* 766,fi:0.0002,di:0.29 */271.85,-1,1.46,8,2,8,8,-1,8,6,8,4,2,0,0,0,0,0,
/* 767,fi:0.0002,di:0.29 */69.17,-1,1.46,6,2,8,12,-1,6,6,8,4,3,0,0,0,0,0,
/* 768,fi:0.0002,di:0.29 */55.21,-1,1.46,6,5,10,8,-1,6,7,10,4,2,0,0,0,0,0,
/* 769,fi:0.0002,di:0.29 */28.19,-1,1.46,2,5,2,4,-1,2,7,2,2,2,0,0,0,0,0,
/* 770,fi:0.0002,di:0.29 */258.33,-1,1.46,3,2,6,12,-1,3,6,6,4,3,0,0,0,0,0,
/* 771,fi:0.0002,di:0.29 */443.01,-1,1.46,5,3,12,8,-1,5,7,12,4,2,0,0,0,0,0,
/* 772,fi:0.0002,di:0.29 */440.41,-1,1.46,0,3,14,8,-1,0,7,14,4,2,0,0,0,0,0,
/* 773,fi:0.0002,di:0.29 */91.81,-1,1.46,4,4,6,4,-1,4,6,6,2,2,0,0,0,0,0,
/* 774,fi:0.0002,di:0.29 */-79.65,1,1.46,2,4,8,12,-1,4,4,4,12,2,0,0,0,0,0,
/* 775,fi:0.0002,di:0.29 */188.09,-1,1.46,13,2,6,12,-1,13,6,6,4,3,0,0,0,0,0,
/* 776,fi:0.0002,di:0.29 */-93.86,1,1.46,9,0,4,6,-1,11,0,2,6,2,0,0,0,0,0,
/* 777,fi:0.0002,di:0.29 */58.18,1,1.46,3,3,8,8,-1,5,3,4,8,2,0,0,0,0,0,
/* 778,fi:0.0002,di:0.29 */363.29,-1,1.46,6,0,8,10,-1,8,0,4,10,2,0,0,0,0,0,
/* 779,fi:0.0002,di:0.29 */-167.27,1,1.46,2,1,14,8,-1,2,3,14,4,2,0,0,0,0,0,
/* 780,fi:0.0002,di:0.29 */31.49,-1,1.46,14,5,2,4,-1,14,7,2,2,2,0,0,0,0,0,
/* 781,fi:0.0002,di:0.29 */471.41,-1,1.46,13,2,4,16,-1,13,6,4,8,2,0,0,0,0,0,
/* 782,fi:0.0002,di:0.29 */27.62,-1,1.46,5,4,2,4,-1,5,6,2,2,2,0,0,0,0,0,
/* 783,fi:0.0002,di:0.27 */77.30,1,1.46,11,15,8,4,-1,11,15,4,2,2,15,17,4,2,2,
/* 784,fi:0.0002,di:0.27 */353.14,-1,1.46,14,1,2,18,-1,14,7,2,6,3,0,0,0,0,0,
/* 785,fi:0.0002,di:0.27 */-75.48,1,1.46,9,4,8,6,-1,11,4,4,6,2,0,0,0,0,0,
/* 786,fi:0.0002,di:0.27 */216.97,-1,1.46,0,3,4,12,-1,0,7,4,4,3,0,0,0,0,0,
/* 787,fi:0.0002,di:0.27 */64.84,-1,1.46,6,5,12,6,-1,6,7,12,2,3,0,0,0,0,0,
/* 788,fi:0.0002,di:0.27 */-39.86,1,1.46,2,3,8,6,-1,4,3,4,6,2,0,0,0,0,0,
/* 789,fi:0.0002,di:0.27 */265.27,-1,1.46,6,0,4,10,-1,8,0,2,10,2,0,0,0,0,0,
/* 790,fi:0.0002,di:0.27 */1383.93,-1,1.46,3,2,16,16,-1,3,6,16,8,2,0,0,0,0,0,
/* 791,fi:0.0002,di:0.27 */217.31,-1,1.46,3,4,4,12,-1,3,8,4,4,3,0,0,0,0,0,
/* 792,fi:0.0002,di:0.27 */71.58,-1,1.46,0,5,14,6,-1,0,7,14,2,3,0,0,0,0,0,
/* 793,fi:0.0002,di:0.27 */5.23,-1,1.46,2,4,4,8,-1,2,6,4,4,2,0,0,0,0,0,
/* 794,fi:0.0002,di:0.27 */230.33,-1,1.45,5,2,6,12,-1,5,6,6,4,3,0,0,0,0,0,
/* 795,fi:0.0002,di:0.27 */73.29,1,1.45,3,2,8,10,-1,5,2,4,10,2,0,0,0,0,0,
/* 796,fi:0.0002,di:0.27 */65.83,-1,1.45,16,5,2,8,-1,16,7,2,4,2,0,0,0,0,0,
/* 797,fi:0.0002,di:0.27 */68.21,1,1.45,3,4,8,8,-1,5,4,4,8,2,0,0,0,0,0,
/* 798,fi:0.0002,di:0.27 */-72.03,1,1.45,5,1,10,6,-1,5,3,10,2,3,0,0,0,0,0,
/* 799,fi:0.0002,di:0.27 */2134.30,-1,1.45,1,0,18,18,-1,1,6,18,6,3,0,0,0,0,0,
/* 800,fi:0.0002,di:0.27 */375.67,-1,1.45,6,2,10,8,-1,6,6,10,4,2,0,0,0,0,0,
/* 801,fi:0.0002,di:0.27 */9.14,1,1.45,4,8,6,4,-1,6,8,2,4,3,0,0,0,0,0,
/* 802,fi:0.0002,di:0.27 */-126.46,1,1.45,0,1,16,8,-1,0,3,16,4,2,0,0,0,0,0,
/* 803,fi:0.0002,di:0.27 */-63.85,1,1.45,1,1,8,8,-1,1,3,8,4,2,0,0,0,0,0,
/* 804,fi:0.0002,di:0.27 */125.55,-1,1.45,7,3,8,12,-1,7,7,8,4,3,0,0,0,0,0,
/* 805,fi:0.0002,di:0.27 */-65.06,1,1.45,10,1,8,8,-1,10,3,8,4,2,0,0,0,0,0,
/* 806,fi:0.0002,di:0.27 */10.83,-1,1.45,11,4,4,8,-1,11,6,4,4,2,0,0,0,0,0,
/* 807,fi:0.0002,di:0.27 */17.04,-1,1.45,3,5,10,6,-1,3,7,10,2,3,0,0,0,0,0,
/* 808,fi:0.0002,di:0.27 */16.31,-1,1.45,5,5,10,8,-1,5,7,10,4,2,0,0,0,0,0,
/* 809,fi:0.0002,di:0.26 */-8.48,1,1.45,2,3,8,2,-1,4,3,4,2,2,0,0,0,0,0,
/* 810,fi:0.0002,di:0.26 */-17.14,1,1.45,2,4,8,6,-1,4,4,4,6,2,0,0,0,0,0,
/* 811,fi:0.0002,di:0.26 */159.91,-1,1.45,1,2,4,8,-1,1,6,4,4,2,0,0,0,0,0,
/* 812,fi:0.0002,di:0.26 */14.44,-1,1.45,6,4,8,4,-1,6,6,8,2,2,0,0,0,0,0,
/* 813,fi:0.0002,di:0.26 */-112.99,1,1.45,1,1,14,8,-1,1,3,14,4,2,0,0,0,0,0,
/* 814,fi:0.0002,di:0.26 */-6.50,-1,1.45,9,4,4,8,-1,9,6,4,4,2,0,0,0,0,0,
/* 815,fi:0.0002,di:0.26 */88.86,-1,1.45,2,5,8,4,-1,2,7,8,2,2,0,0,0,0,0,
/* 816,fi:0.0002,di:0.26 */21.59,-1,1.45,16,4,2,4,-1,16,6,2,2,2,0,0,0,0,0,
/* 817,fi:0.0002,di:0.26 */254.37,-1,1.45,15,2,2,16,-1,15,6,2,8,2,0,0,0,0,0,
/* 818,fi:0.0002,di:0.26 */111.41,-1,1.45,11,3,4,12,-1,11,7,4,4,3,0,0,0,0,0,
/* 819,fi:0.0002,di:0.26 */-18.33,1,1.45,6,0,2,8,-1,6,2,2,4,2,0,0,0,0,0,
/* 820,fi:0.0002,di:0.26 */854.43,-1,1.45,9,2,10,16,-1,9,6,10,8,2,0,0,0,0,0,
/* 821,fi:0.0002,di:0.26 */3.06,-1,1.45,3,4,2,4,-1,3,6,2,2,2,0,0,0,0,0,
/* 822,fi:0.0002,di:0.26 */-44.94,1,1.45,4,0,6,16,-1,6,0,2,16,3,0,0,0,0,0,
/* 823,fi:0.0002,di:0.26 */32.32,1,1.45,0,0,8,4,-1,0,0,4,2,2,4,2,4,2,2,
/* 824,fi:0.0002,di:0.26 */15.25,-1,1.45,6,5,10,6,-1,6,7,10,2,3,0,0,0,0,0,
/* 825,fi:0.0002,di:0.26 */4.17,1,1.45,11,1,6,6,-1,11,3,6,2,3,0,0,0,0,0,
/* 826,fi:0.0002,di:0.26 */-44.10,1,1.45,15,1,2,8,-1,15,3,2,4,2,0,0,0,0,0,
/* 827,fi:0.0002,di:0.26 */49.27,-1,1.45,8,5,8,4,-1,8,7,8,2,2,0,0,0,0,0,
/* 828,fi:0.0002,di:0.26 */103.42,1,1.45,0,2,8,6,-1,0,4,8,2,3,0,0,0,0,0,
/* 829,fi:0.0002,di:0.26 */223.80,-1,1.45,6,1,4,16,-1,8,1,2,16,2,0,0,0,0,0,
/* 830,fi:0.0002,di:0.26 */460.94,-1,1.45,13,4,6,12,-1,13,8,6,4,3,0,0,0,0,0,
/* 831,fi:0.0002,di:0.25 */20.87,-1,1.45,13,4,4,8,-1,13,6,4,4,2,0,0,0,0,0,
/* 832,fi:0.0002,di:0.25 */93.68,-1,1.45,0,5,10,4,-1,0,7,10,2,2,0,0,0,0,0,
/* 833,fi:0.0002,di:0.25 */101.14,-1,1.45,6,3,4,6,-1,8,3,2,6,2,0,0,0,0,0,
/* 834,fi:0.0002,di:0.25 */-28.46,-1,1.45,7,4,4,4,-1,9,4,2,4,2,0,0,0,0,0,
/* 835,fi:0.0002,di:0.25 */1345.74,-1,1.44,2,2,16,16,-1,2,6,16,8,2,0,0,0,0,0,
/* 836,fi:0.0002,di:0.25 */36.11,-1,1.44,15,5,2,4,-1,15,7,2,2,2,0,0,0,0,0,
/* 837,fi:0.0002,di:0.25 */31.43,-1,1.44,7,6,6,10,-1,9,6,2,10,3,0,0,0,0,0,
/* 838,fi:0.0002,di:0.25 */111.41,-1,1.44,1,2,6,12,-1,1,6,6,4,3,0,0,0,0,0,
/* 839,fi:0.0002,di:0.25 */62.37,-1,1.44,10,6,8,6,-1,10,8,8,2,3,0,0,0,0,0,
/* 840,fi:0.0002,di:0.25 */313.19,-1,1.44,2,3,8,8,-1,2,7,8,4,2,0,0,0,0,0,
/* 841,fi:0.0002,di:0.25 */-16.00,1,1.44,2,3,8,4,-1,4,3,4,4,2,0,0,0,0,0,
/* 842,fi:0.0002,di:0.25 */-133.07,1,1.44,9,3,4,8,-1,11,3,2,8,2,0,0,0,0,0,
/* 843,fi:0.0002,di:0.25 */-31.89,1,1.44,4,4,6,14,-1,6,4,2,14,3,0,0,0,0,0,
/* 844,fi:0.0002,di:0.25 */451.11,-1,1.44,2,2,10,8,-1,2,6,10,4,2,0,0,0,0,0,
/* 845,fi:0.0002,di:0.25 */32.81,-1,1.44,10,6,6,6,-1,10,8,6,2,3,0,0,0,0,0,
/* 846,fi:0.0002,di:0.25 */36.76,-1,1.44,4,5,2,8,-1,4,7,2,4,2,0,0,0,0,0,
/* 847,fi:0.0002,di:0.25 */-107.96,1,1.44,8,1,10,8,-1,8,3,10,4,2,0,0,0,0,0,
/* 848,fi:0.0002,di:0.25 */44.23,-1,1.44,2,5,2,8,-1,2,7,2,4,2,0,0,0,0,0,
/* 849,fi:0.0002,di:0.25 */1240.42,-1,1.44,1,2,16,16,-1,1,6,16,8,2,0,0,0,0,0,
/* 850,fi:0.0002,di:0.25 */-16.25,-1,1.44,8,2,6,12,-1,8,6,6,4,3,0,0,0,0,0,
/* 851,fi:0.0002,di:0.25 */145.78,-1,1.44,12,2,6,12,-1,12,6,6,4,3,0,0,0,0,0,
/* 852,fi:0.0002,di:0.25 */232.52,-1,1.44,16,2,2,16,-1,16,6,2,8,2,0,0,0,0,0,
/* 853,fi:0.0002,di:0.25 */-35.36,1,1.44,4,3,6,16,-1,6,3,2,16,3,0,0,0,0,0,
/* 854,fi:0.0002,di:0.25 */-97.07,1,1.44,2,3,8,10,-1,4,3,4,10,2,0,0,0,0,0,
/* 855,fi:0.0002,di:0.25 */73.99,1,1.44,2,2,6,6,-1,2,4,6,2,3,0,0,0,0,0,
/* 856,fi:0.0002,di:0.25 */73.35,1,1.44,7,2,12,6,-1,7,4,12,2,3,0,0,0,0,0,
/* 857,fi:0.0002,di:0.25 */44.10,-1,1.44,5,5,6,8,-1,5,7,6,4,2,0,0,0,0,0,
/* 858,fi:0.0002,di:0.25 */684.56,-1,1.44,13,1,4,18,-1,13,7,4,6,3,0,0,0,0,0,
/* 859,fi:0.0002,di:0.25 */286.49,-1,1.44,3,3,6,8,-1,3,7,6,4,2,0,0,0,0,0,
/* 860,fi:0.0002,di:0.25 */268.68,-1,1.44,1,2,8,12,-1,1,6,8,4,3,0,0,0,0,0,
/* 861,fi:0.0002,di:0.25 */35.56,-1,1.44,4,5,10,8,-1,4,7,10,4,2,0,0,0,0,0,
/* 862,fi:0.0002,di:0.25 */373.30,-1,1.44,12,4,6,12,-1,12,8,6,4,3,0,0,0,0,0,
/* 863,fi:0.0002,di:0.25 */101.80,-1,1.44,12,5,6,8,-1,12,7,6,4,2,0,0,0,0,0,
/* 864,fi:0.0002,di:0.25 */66.69,-1,1.44,9,3,4,16,-1,9,3,2,8,2,11,11,2,8,2,
/* 865,fi:0.0002,di:0.25 */214.02,-1,1.44,6,1,4,12,-1,8,1,2,12,2,0,0,0,0,0,
/* 866,fi:0.0002,di:0.25 */-91.84,1,1.44,9,4,4,6,-1,11,4,2,6,2,0,0,0,0,0,
/* 867,fi:0.0002,di:0.25 */35.31,1,1.44,3,1,4,6,-1,3,3,4,2,3,0,0,0,0,0,
/* 868,fi:0.0002,di:0.25 */-61.48,1,1.44,8,0,6,8,-1,8,2,6,4,2,0,0,0,0,0,
/* 869,fi:0.0002,di:0.25 */283.19,-1,1.44,15,4,4,12,-1,15,8,4,4,3,0,0,0,0,0,
/* 870,fi:0.0002,di:0.25 */50.78,1,1.44,1,2,12,6,-1,1,4,12,2,3,0,0,0,0,0,
/* 871,fi:0.0002,di:0.25 */425.05,-1,1.44,0,3,10,8,-1,0,7,10,4,2,0,0,0,0,0,
/* 872,fi:0.0002,di:0.25 */-199.58,1,1.44,2,0,8,14,-1,4,0,4,14,2,0,0,0,0,0,
/* 873,fi:0.0002,di:0.25 */111.99,-1,1.44,5,3,2,12,-1,5,7,2,4,3,0,0,0,0,0,
/* 874,fi:0.0002,di:0.25 */-110.81,1,1.44,2,1,10,8,-1,2,3,10,4,2,0,0,0,0,0,
/* 875,fi:0.0002,di:0.25 */319.39,-1,1.44,2,3,10,8,-1,2,7,10,4,2,0,0,0,0,0,
/* 876,fi:0.0002,di:0.25 */-66.05,1,1.43,6,0,6,8,-1,6,2,6,4,2,0,0,0,0,0,
/* 877,fi:0.0002,di:0.25 */49.88,-1,1.43,1,5,12,6,-1,1,7,12,2,3,0,0,0,0,0,
/* 878,fi:0.0002,di:0.25 */84.59,1,1.43,5,2,12,6,-1,5,4,12,2,3,0,0,0,0,0,
/* 879,fi:0.0002,di:0.25 */-28.53,1,1.43,13,0,2,8,-1,13,2,2,4,2,0,0,0,0,0,
/* 880,fi:0.0002,di:0.25 */56.38,-1,1.43,6,4,4,8,-1,6,6,4,4,2,0,0,0,0,0,
/* 881,fi:0.0002,di:0.25 */57.94,-1,1.43,6,3,4,4,-1,8,3,2,4,2,0,0,0,0,0,
/* 882,fi:0.0002,di:0.25 */51.58,-1,1.43,7,2,6,2,-1,9,2,2,2,3,0,0,0,0,0,
/* 883,fi:0.0002,di:0.25 */-37.72,1,1.43,9,7,2,8,-1,9,11,2,4,2,0,0,0,0,0,
/* 884,fi:0.0002,di:0.25 */-46.65,1,1.43,9,2,8,6,-1,11,2,4,6,2,0,0,0,0,0,
/* 885,fi:0.0002,di:0.25 */406.20,-1,1.43,0,3,12,8,-1,0,7,12,4,2,0,0,0,0,0,
/* 886,fi:0.0002,di:0.25 */254.77,-1,1.43,6,1,8,8,-1,8,1,4,8,2,0,0,0,0,0,
/* 887,fi:0.0002,di:0.25 */-17.34,-1,1.43,7,5,8,8,-1,7,7,8,4,2,0,0,0,0,0,
/* 888,fi:0.0002,di:0.25 */83.82,-1,1.43,12,2,4,12,-1,12,6,4,4,3,0,0,0,0,0,
/* 889,fi:0.0002,di:0.24 */77.80,1,1.43,5,0,8,4,-1,5,2,8,2,2,0,0,0,0,0,
/* 890,fi:0.0002,di:0.24 */363.85,-1,1.43,2,2,8,8,-1,2,6,8,4,2,0,0,0,0,0,
/* 891,fi:0.0002,di:0.24 */-49.27,1,1.43,9,1,4,4,-1,11,1,2,4,2,0,0,0,0,0,
/* 892,fi:0.0002,di:0.24 */88.42,-1,1.43,6,2,4,4,-1,8,2,2,4,2,0,0,0,0,0,
/* 893,fi:0.0002,di:0.24 */461.70,-1,1.43,1,0,4,18,-1,1,6,4,6,3,0,0,0,0,0,
/* 894,fi:0.0002,di:0.24 */591.00,-1,1.43,2,1,4,18,-1,2,7,4,6,3,0,0,0,0,0,
/* 895,fi:0.0002,di:0.24 */317.90,-1,1.43,15,0,2,18,-1,15,6,2,6,3,0,0,0,0,0,
/* 896,fi:0.0002,di:0.24 */63.99,1,1.43,10,2,8,6,-1,10,4,8,2,3,0,0,0,0,0,
/* 897,fi:0.0002,di:0.24 */-43.23,1,1.43,7,0,6,8,-1,7,2,6,4,2,0,0,0,0,0,
/* 898,fi:0.0002,di:0.24 */50.13,1,1.43,3,5,8,6,-1,5,5,4,6,2,0,0,0,0,0,
/* 899,fi:0.0002,di:0.24 */-155.31,1,1.43,9,1,8,14,-1,11,1,4,14,2,0,0,0,0,0,
/* 900,fi:0.0002,di:0.24 */342.82,-1,1.43,2,3,12,8,-1,2,7,12,4,2,0,0,0,0,0,
/* 901,fi:0.0002,di:0.24 */-39.47,1,1.43,2,4,8,8,-1,4,4,4,8,2,0,0,0,0,0,
/* 902,fi:0.0002,di:0.24 */8.39,-1,1.43,10,5,6,8,-1,10,7,6,4,2,0,0,0,0,0,
/* 903,fi:0.0002,di:0.24 */79.96,1,1.43,0,2,14,6,-1,0,4,14,2,3,0,0,0,0,0,
/* 904,fi:0.0002,di:0.24 */-94.18,1,1.43,2,1,8,8,-1,2,3,8,4,2,0,0,0,0,0,
/* 905,fi:0.0002,di:0.24 */-4.10,-1,1.43,9,5,6,8,-1,9,7,6,4,2,0,0,0,0,0,
/* 906,fi:0.0002,di:0.24 */34.19,-1,1.43,9,2,4,16,-1,9,2,2,8,2,11,10,2,8,2,
/* 907,fi:0.0002,di:0.24 */-45.46,1,1.43,2,1,6,8,-1,2,3,6,4,2,0,0,0,0,0,
/* 908,fi:0.0002,di:0.24 */73.25,1,1.43,4,2,12,6,-1,4,4,12,2,3,0,0,0,0,0,
/* 909,fi:0.0002,di:0.24 */-32.96,1,1.43,1,1,6,8,-1,1,3,6,4,2,0,0,0,0,0,
/* 910,fi:0.0002,di:0.24 */-140.49,1,1.43,9,2,4,10,-1,11,2,2,10,2,0,0,0,0,0,
/* 911,fi:0.0002,di:0.24 */-170.46,1,1.43,2,0,8,10,-1,4,0,4,10,2,0,0,0,0,0,
/* 912,fi:0.0002,di:0.24 */192.95,-1,1.43,6,0,4,8,-1,8,0,2,8,2,0,0,0,0,0,
/* 913,fi:0.0002,di:0.24 */68.36,-1,1.43,3,5,4,8,-1,3,7,4,4,2,0,0,0,0,0,
/* 914,fi:0.0002,di:0.24 */227.16,-1,1.43,1,4,4,12,-1,1,8,4,4,3,0,0,0,0,0,
/* 915,fi:0.0002,di:0.24 */-10.43,1,1.43,4,3,6,6,-1,6,3,2,6,3,0,0,0,0,0,
/* 916,fi:0.0002,di:0.24 */18.76,-1,1.42,2,5,10,6,-1,2,7,10,2,3,0,0,0,0,0,
/* 917,fi:0.0002,di:0.24 */34.10,1,1.42,3,2,6,6,-1,3,4,6,2,3,0,0,0,0,0,
/* 918,fi:0.0002,di:0.24 */68.74,-1,1.42,6,6,6,6,-1,6,8,6,2,3,0,0,0,0,0,
/* 919,fi:0.0002,di:0.24 */747.38,-1,1.42,1,4,18,12,-1,1,8,18,4,3,0,0,0,0,0,
/* 920,fi:0.0002,di:0.24 */-7.32,-1,1.42,10,4,4,4,-1,10,6,4,2,2,0,0,0,0,0,
/* 921,fi:0.0002,di:0.24 */174.66,-1,1.42,6,2,4,14,-1,8,2,2,14,2,0,0,0,0,0,
/* 922,fi:0.0002,di:0.24 */-31.68,1,1.42,11,1,8,8,-1,11,3,8,4,2,0,0,0,0,0,
/* 923,fi:0.0002,di:0.24 */-21.85,-1,1.42,7,5,4,2,-1,9,5,2,2,2,0,0,0,0,0,
/* 924,fi:0.0002,di:0.24 */728.60,-1,1.43,10,2,8,16,-1,10,6,8,8,2,0,0,0,0,0,
/* 925,fi:0.0002,di:0.24 */22.95,-1,1.43,0,4,6,8,-1,0,6,6,4,2,0,0,0,0,0,
/* 926,fi:0.0002,di:0.24 */183.51,-1,1.43,16,3,2,16,-1,16,7,2,8,2,0,0,0,0,0,
/* 927,fi:0.0002,di:0.24 */249.62,-1,1.43,14,0,2,18,-1,14,6,2,6,3,0,0,0,0,0,
/* 928,fi:0.0002,di:0.24 */135.16,-1,1.42,0,6,10,6,-1,0,8,10,2,3,0,0,0,0,0,
/* 929,fi:0.0002,di:0.23 */60.53,-1,1.42,6,1,6,10,-1,8,1,2,10,3,0,0,0,0,0,
/* 930,fi:0.0002,di:0.23 */74.55,-1,1.42,3,5,10,4,-1,3,7,10,2,2,0,0,0,0,0,
/* 931,fi:0.0002,di:0.23 */316.02,-1,1.42,7,3,10,8,-1,7,7,10,4,2,0,0,0,0,0,
/* 932,fi:0.0002,di:0.23 */33.26,-1,1.42,14,2,2,12,-1,14,6,2,4,3,0,0,0,0,0,
/* 933,fi:0.0002,di:0.23 */1093.92,-1,1.42,0,2,16,16,-1,0,6,16,8,2,0,0,0,0,0,
/* 934,fi:0.0002,di:0.23 */104.55,1,1.42,11,2,8,6,-1,11,4,8,2,3,0,0,0,0,0,
/* 935,fi:0.0002,di:0.23 */13.29,1,1.42,4,4,6,12,-1,6,4,2,12,3,0,0,0,0,0,
/* 936,fi:0.0002,di:0.23 */384.08,-1,1.42,11,4,8,12,-1,11,8,8,4,3,0,0,0,0,0,
/* 937,fi:0.0002,di:0.23 */111.91,-1,1.42,0,5,8,8,-1,0,7,8,4,2,0,0,0,0,0,
/* 938,fi:0.0002,di:0.23 */53.28,-1,1.42,11,5,6,8,-1,11,7,6,4,2,0,0,0,0,0,
/* 939,fi:0.0002,di:0.23 */-34.52,1,1.42,9,3,8,6,-1,11,3,4,6,2,0,0,0,0,0,
/* 940,fi:0.0002,di:0.23 */59.50,-1,1.42,2,2,4,12,-1,2,6,4,4,3,0,0,0,0,0,
/* 941,fi:0.0002,di:0.23 */339.88,-1,1.42,5,2,10,8,-1,5,6,10,4,2,0,0,0,0,0,
/* 942,fi:0.0002,di:0.23 */49.95,-1,1.42,14,5,2,8,-1,14,7,2,4,2,0,0,0,0,0,
/* 943,fi:0.0002,di:0.23 */124.72,-1,1.42,2,4,2,12,-1,2,8,2,4,3,0,0,0,0,0,
/* 944,fi:0.0002,di:0.23 */91.50,-1,1.42,13,2,4,12,-1,13,6,4,4,3,0,0,0,0,0,
/* 945,fi:0.0002,di:0.23 */312.84,-1,1.42,1,4,6,12,-1,1,8,6,4,3,0,0,0,0,0,
/* 946,fi:0.0002,di:0.23 */-180.66,1,1.42,9,1,8,16,-1,11,1,4,16,2,0,0,0,0,0,
/* 947,fi:0.0002,di:0.23 */-116.16,-1,1.42,7,2,4,10,-1,9,2,2,10,2,0,0,0,0,0,
/* 948,fi:0.0002,di:0.23 */1001.67,-1,1.42,12,1,6,18,-1,12,7,6,6,3,0,0,0,0,0,
/* 949,fi:0.0002,di:0.23 */-70.87,1,1.42,1,1,12,8,-1,1,3,12,4,2,0,0,0,0,0,
/* 950,fi:0.0002,di:0.23 */-166.89,1,1.42,9,2,8,14,-1,11,2,4,14,2,0,0,0,0,0,
/* 951,fi:0.0002,di:0.23 */330.30,-1,1.42,15,3,4,16,-1,15,7,4,8,2,0,0,0,0,0,
/* 952,fi:0.0002,di:0.23 */28.83,-1,1.42,13,2,2,12,-1,13,6,2,4,3,0,0,0,0,0,
/* 953,fi:0.0002,di:0.23 */1137.85,-1,1.42,9,0,10,18,-1,9,6,10,6,3,0,0,0,0,0,
/* 954,fi:0.0002,di:0.23 */96.35,-1,1.42,1,3,2,12,-1,1,7,2,4,3,0,0,0,0,0,
/* 955,fi:0.0002,di:0.23 */-58.48,1,1.41,9,1,10,8,-1,9,3,10,4,2,0,0,0,0,0,
/* 956,fi:0.0002,di:0.23 */51.45,-1,1.41,7,6,6,6,-1,7,8,6,2,3,0,0,0,0,0,
/* 957,fi:0.0002,di:0.23 */270.29,-1,1.41,17,0,2,18,-1,17,6,2,6,3,0,0,0,0,0,
/* 958,fi:0.0002,di:0.23 */43.81,-1,1.41,7,6,6,12,-1,9,6,2,12,3,0,0,0,0,0,
/* 959,fi:0.0002,di:0.23 */77.30,1,1.41,3,3,8,10,-1,5,3,4,10,2,0,0,0,0,0,
/* 960,fi:0.0002,di:0.23 */68.16,1,1.41,3,4,8,6,-1,5,4,4,6,2,0,0,0,0,0,
/* 961,fi:0.0002,di:0.22 */86.46,1,1.41,6,0,8,4,-1,6,2,8,2,2,0,0,0,0,0,
/* 962,fi:0.0002,di:0.22 */107.54,1,1.41,3,0,10,4,-1,3,2,10,2,2,0,0,0,0,0,
/* 963,fi:0.0002,di:0.22 */110.39,-1,1.41,5,3,8,12,-1,5,7,8,4,3,0,0,0,0,0,
/* 964,fi:0.0002,di:0.22 */218.55,-1,1.41,14,2,2,16,-1,14,6,2,8,2,0,0,0,0,0,
/* 965,fi:0.0002,di:0.22 */151.22,-1,1.41,17,2,2,16,-1,17,6,2,8,2,0,0,0,0,0,
/* 966,fi:0.0002,di:0.22 */-79.15,1,1.41,3,1,6,8,-1,3,3,6,4,2,0,0,0,0,0,
/* 967,fi:0.0002,di:0.22 */-175.60,1,1.41,9,0,4,12,-1,11,0,2,12,2,0,0,0,0,0,
/* 968,fi:0.0002,di:0.22 */-141.24,1,1.41,9,2,8,12,-1,11,2,4,12,2,0,0,0,0,0,
/* 969,fi:0.0002,di:0.22 */4.08,-1,1.41,10,5,8,6,-1,10,7,8,2,3,0,0,0,0,0,
/* 970,fi:0.0002,di:0.22 */365.50,-1,1.41,4,2,10,8,-1,4,6,10,4,2,0,0,0,0,0,
/* 971,fi:0.0002,di:0.22 */91.49,1,1.41,0,1,8,6,-1,0,3,8,2,3,0,0,0,0,0,
/* 972,fi:0.0002,di:0.22 */81.25,1,1.41,2,2,4,6,-1,2,4,4,2,3,0,0,0,0,0,
/* 973,fi:0.0002,di:0.22 */-20.97,-1,1.41,5,15,2,4,-1,5,17,2,2,2,0,0,0,0,0,
/* 974,fi:0.0002,di:0.22 */145.59,-1,1.43,2,2,6,12,-1,2,6,6,4,3,0,0,0,0,0,
/* 975,fi:0.0002,di:0.22 */318.57,-1,1.42,16,0,2,18,-1,16,6,2,6,3,0,0,0,0,0,
/* 976,fi:0.0002,di:0.22 */40.28,-1,1.42,2,2,2,8,-1,2,6,2,4,2,0,0,0,0,0,
/* 977,fi:0.0002,di:0.21 */405.50,-1,1.42,14,1,4,12,-1,14,7,4,6,2,0,0,0,0,0,
/* 978,fi:0.0002,di:0.21 */178.11,-1,1.42,0,2,8,12,-1,0,6,8,4,3,0,0,0,0,0,
/* 979,fi:0.0002,di:0.21 */288.87,-1,1.42,3,2,6,8,-1,3,6,6,4,2,0,0,0,0,0,
/* 980,fi:0.0002,di:0.21 */40.67,-1,1.42,0,5,12,6,-1,0,7,12,2,3,0,0,0,0,0,
/* 981,fi:0.0002,di:0.21 */734.29,-1,1.42,12,0,6,18,-1,12,6,6,6,3,0,0,0,0,0,
/* 982,fi:0.0002,di:0.21 */-19.08,-1,1.42,2,4,2,4,-1,2,6,2,2,2,0,0,0,0,0,
/* 983,fi:0.0002,di:0.21 */55.09,-1,1.42,7,5,12,6,-1,7,7,12,2,3,0,0,0,0,0,
/* 984,fi:0.0002,di:0.21 */9.56,-1,1.42,8,5,10,6,-1,8,7,10,2,3,0,0,0,0,0,
/* 985,fi:0.0000,di:0.21 */-6.96,1,1.41,3,9,4,2,-1,5,9,2,2,2,0,0,0,0,0,
// end of 14-th stage with 986 classifiers
/* 14 stages: */
//0,1,8,21,33,42,62,97,148,267,308,339,361,449,986
  };
  static CvMat haarclassifier_face20 =
      cvMat(986,18,CV_32F,haarclassifier_face20_data);
  return &haarclassifier_face20;
}

// 10 stages, .85,.96
//0,1,2,10,29,73,103,160,196,201,316
CvMat * get_haarclassifier_face20_v1()
{
  static float haarclassifier_face20_data[]={
/* 0,fi:0.2945,di:0.92 */68.55,-1,1.47,7,8,6,2,-1,9,8,2,2,3,0,0,0,0,0,
//--
/* 1,fi:0.1110,di:0.86 */1214.64,-1,1.47,1,2,18,12,-1,1,6,18,4,3,0,0,0,0,0,
//--
/* 2,fi:0.0885,di:0.83 */482.81,-1,1.46,3,4,16,8,-1,3,6,16,4,2,0,0,0,0,0,
/* 3,fi:0.0670,di:0.81 */366.14,-1,1.45,1,4,18,4,-1,1,6,18,2,2,0,0,0,0,0,
/* 4,fi:0.0655,di:0.80 */325.42,-1,1.45,3,4,16,4,-1,3,6,16,2,2,0,0,0,0,0,
/* 5,fi:0.0655,di:0.80 */1121.91,-1,1.45,3,2,16,12,-1,3,6,16,4,3,0,0,0,0,0,
/* 6,fi:0.0645,di:0.80 */345.27,-1,1.45,0,4,18,4,-1,0,6,18,2,2,0,0,0,0,0,
/* 7,fi:0.0635,di:0.79 */522.58,-1,1.44,1,4,18,8,-1,1,6,18,4,2,0,0,0,0,0,
/* 8,fi:0.0625,di:0.79 */398.03,-1,1.44,4,4,14,8,-1,4,6,14,4,2,0,0,0,0,0,
/* 9,fi:0.0480,di:0.77 */432.78,-1,1.44,7,1,6,10,-1,9,1,2,10,3,0,0,0,0,0,
//--
/* 10,fi:0.0480,di:0.77 */442.32,-1,1.44,2,4,16,8,-1,2,6,16,4,2,0,0,0,0,0,
/* 11,fi:0.0470,di:0.77 */519.28,-1,1.44,0,4,18,8,-1,0,6,18,4,2,0,0,0,0,0,
/* 12,fi:0.0460,di:0.76 */319.55,-1,1.43,7,3,6,8,-1,9,3,2,8,3,0,0,0,0,0,
/* 13,fi:0.0460,di:0.76 */322.35,-1,1.43,2,4,16,4,-1,2,6,16,2,2,0,0,0,0,0,
/* 14,fi:0.0360,di:0.73 */154.40,-1,1.42,14,4,4,4,-1,14,6,4,2,2,0,0,0,0,0,
/* 15,fi:0.0340,di:0.73 */369.98,-1,1.42,7,2,6,8,-1,9,2,2,8,3,0,0,0,0,0,
/* 16,fi:0.0315,di:0.72 */417.10,-1,1.42,14,2,4,12,-1,14,6,4,4,3,0,0,0,0,0,
/* 17,fi:0.0310,di:0.72 */378.04,-1,1.42,3,4,14,8,-1,3,6,14,4,2,0,0,0,0,0,
/* 18,fi:0.0310,di:0.71 */235.09,-1,1.42,7,4,6,6,-1,9,4,2,6,3,0,0,0,0,0,
/* 19,fi:0.0305,di:0.71 */255.36,-1,1.41,4,4,14,4,-1,4,6,14,2,2,0,0,0,0,0,
/* 20,fi:0.0305,di:0.71 */228.17,-1,1.41,7,5,6,6,-1,9,5,2,6,3,0,0,0,0,0,
/* 21,fi:0.0295,di:0.70 */399.22,-1,1.41,15,2,4,12,-1,15,6,4,4,3,0,0,0,0,0,
/* 22,fi:0.0285,di:0.70 */452.61,-1,1.41,14,2,4,8,-1,14,6,4,4,2,0,0,0,0,0,
/* 23,fi:0.0285,di:0.70 */303.30,-1,1.41,1,4,16,4,-1,1,6,16,2,2,0,0,0,0,0,
/* 24,fi:0.0265,di:0.69 */239.07,-1,1.41,15,2,2,8,-1,15,6,2,4,2,0,0,0,0,0,
/* 25,fi:0.0265,di:0.69 */190.92,-1,1.40,13,4,6,4,-1,13,6,6,2,2,0,0,0,0,0,
/* 26,fi:0.0265,di:0.69 */1227.77,-1,1.40,0,2,18,12,-1,0,6,18,4,3,0,0,0,0,0,
/* 27,fi:0.0265,di:0.68 */921.69,-1,1.40,4,2,14,12,-1,4,6,14,4,3,0,0,0,0,0,
/* 28,fi:0.0240,di:0.68 */465.13,-1,1.40,7,2,6,14,-1,9,2,2,14,3,0,0,0,0,0,
//--
/* 29,fi:0.0240,di:0.68 */1087.02,-1,1.40,2,2,16,12,-1,2,6,16,4,3,0,0,0,0,0,
/* 30,fi:0.0240,di:0.68 */576.34,-1,1.40,13,2,6,12,-1,13,6,6,4,3,0,0,0,0,0,
/* 31,fi:0.0240,di:0.68 */422.76,-1,1.39,1,4,16,8,-1,1,6,16,4,2,0,0,0,0,0,
/* 32,fi:0.0235,di:0.68 */407.62,-1,1.39,5,4,14,8,-1,5,6,14,4,2,0,0,0,0,0,
/* 33,fi:0.0230,di:0.68 */416.45,-1,1.39,7,3,6,14,-1,9,3,2,14,3,0,0,0,0,0,
/* 34,fi:0.0230,di:0.68 */616.89,-1,1.39,11,2,8,12,-1,11,6,8,4,3,0,0,0,0,0,
/* 35,fi:0.0225,di:0.68 */446.92,-1,1.39,7,0,6,10,-1,9,0,2,10,3,0,0,0,0,0,
/* 36,fi:0.0220,di:0.67 */73.95,-1,1.39,15,4,2,4,-1,15,6,2,2,2,0,0,0,0,0,
/* 37,fi:0.0215,di:0.67 */271.68,-1,1.39,4,4,12,8,-1,4,6,12,4,2,0,0,0,0,0,
/* 38,fi:0.0215,di:0.67 */464.85,-1,1.38,7,1,6,14,-1,9,1,2,14,3,0,0,0,0,0,
/* 39,fi:0.0215,di:0.67 */1034.11,-1,1.38,1,2,16,12,-1,1,6,16,4,3,0,0,0,0,0,
/* 40,fi:0.0215,di:0.67 */911.47,-1,1.37,5,2,14,12,-1,5,6,14,4,3,0,0,0,0,0,
/* 41,fi:0.0215,di:0.67 */161.28,-1,1.37,10,4,8,4,-1,10,6,8,2,2,0,0,0,0,0,
/* 42,fi:0.0205,di:0.66 */397.59,-1,1.37,15,2,4,8,-1,15,6,4,4,2,0,0,0,0,0,
/* 43,fi:0.0205,di:0.66 */364.41,-1,1.37,7,2,6,10,-1,9,2,2,10,3,0,0,0,0,0,
/* 44,fi:0.0205,di:0.66 */324.87,-1,1.37,7,3,6,10,-1,9,3,2,10,3,0,0,0,0,0,
/* 45,fi:0.0200,di:0.66 */224.77,-1,1.37,2,4,14,4,-1,2,6,14,2,2,0,0,0,0,0,
/* 46,fi:0.0200,di:0.66 */420.18,-1,1.37,7,2,6,12,-1,9,2,2,12,3,0,0,0,0,0,
/* 47,fi:0.0200,di:0.66 */694.46,-1,1.37,9,2,10,12,-1,9,6,10,4,3,0,0,0,0,0,
/* 48,fi:0.0195,di:0.66 */336.38,-1,1.37,2,4,14,8,-1,2,6,14,4,2,0,0,0,0,0,
/* 49,fi:0.0195,di:0.66 */386.52,-1,1.36,0,4,16,8,-1,0,6,16,4,2,0,0,0,0,0,
/* 50,fi:0.0195,di:0.66 */248.53,-1,1.36,3,4,14,4,-1,3,6,14,2,2,0,0,0,0,0,
/* 51,fi:0.0190,di:0.66 */344.42,-1,1.36,7,3,6,12,-1,9,3,2,12,3,0,0,0,0,0,
/* 52,fi:0.0170,di:0.65 */234.19,-1,1.36,14,2,2,8,-1,14,6,2,4,2,0,0,0,0,0,
/* 53,fi:0.0165,di:0.64 */881.48,-1,1.36,14,0,4,18,-1,14,6,4,6,3,0,0,0,0,0,
/* 54,fi:0.0165,di:0.63 */449.95,-1,1.36,12,2,6,12,-1,12,6,6,4,3,0,0,0,0,0,
/* 55,fi:0.0165,di:0.63 */455.57,-1,1.36,7,0,6,12,-1,9,0,2,12,3,0,0,0,0,0,
/* 56,fi:0.0165,di:0.63 */154.48,-1,1.35,12,4,6,4,-1,12,6,6,2,2,0,0,0,0,0,
/* 57,fi:0.0165,di:0.63 */352.43,-1,1.35,7,1,6,8,-1,9,1,2,8,3,0,0,0,0,0,
/* 58,fi:0.0165,di:0.63 */858.39,-1,1.35,3,2,14,12,-1,3,6,14,4,3,0,0,0,0,0,
/* 59,fi:0.0165,di:0.63 */534.30,-1,1.35,7,1,6,16,-1,9,1,2,16,3,0,0,0,0,0,
/* 60,fi:0.0165,di:0.63 */286.82,-1,1.35,5,4,12,8,-1,5,6,12,4,2,0,0,0,0,0,
/* 61,fi:0.0165,di:0.63 */267.13,-1,1.35,0,4,16,4,-1,0,6,16,2,2,0,0,0,0,0,
/* 62,fi:0.0165,di:0.63 */210.81,-1,1.35,15,2,2,12,-1,15,6,2,4,3,0,0,0,0,0,
/* 63,fi:0.0155,di:0.63 */329.52,-1,1.35,1,4,14,8,-1,1,6,14,4,2,0,0,0,0,0,
/* 64,fi:0.0155,di:0.63 */348.90,-1,1.34,13,2,4,12,-1,13,6,4,4,3,0,0,0,0,0,
/* 65,fi:0.0150,di:0.62 */65.51,-1,1.34,14,4,2,4,-1,14,6,2,2,2,0,0,0,0,0,
/* 66,fi:0.0150,di:0.62 */195.37,-1,1.35,11,4,8,4,-1,11,6,8,2,2,0,0,0,0,0,
/* 67,fi:0.0150,di:0.62 */247.91,-1,1.34,8,4,10,8,-1,8,6,10,4,2,0,0,0,0,0,
/* 68,fi:0.0150,di:0.62 */454.18,-1,1.34,7,1,6,12,-1,9,1,2,12,3,0,0,0,0,0,
/* 69,fi:0.0150,di:0.61 */760.29,-1,1.34,15,0,4,18,-1,15,6,4,6,3,0,0,0,0,0,
/* 70,fi:0.0125,di:0.61 */311.95,-1,1.34,0,4,12,8,-1,0,6,12,4,2,0,0,0,0,0,
/* 71,fi:0.0125,di:0.61 */519.72,-1,1.34,10,2,8,12,-1,10,6,8,4,3,0,0,0,0,0,
/* 72,fi:0.0120,di:0.60 */576.21,-1,1.34,13,2,6,8,-1,13,6,6,4,2,0,0,0,0,0,
//--
/* 73,fi:0.0120,di:0.60 */1072.16,-1,1.34,1,2,18,8,-1,1,6,18,4,2,0,0,0,0,0,
/* 74,fi:0.0120,di:0.60 */345.36,-1,1.34,7,4,6,12,-1,9,4,2,12,3,0,0,0,0,0,
/* 75,fi:0.0120,di:0.60 */194.74,-1,1.34,16,2,2,8,-1,16,6,2,4,2,0,0,0,0,0,
/* 76,fi:0.0120,di:0.60 */313.41,-1,1.34,7,4,12,8,-1,7,6,12,4,2,0,0,0,0,0,
/* 77,fi:0.0115,di:0.60 */151.38,-1,1.34,7,6,6,4,-1,9,6,2,4,3,0,0,0,0,0,
/* 78,fi:0.0115,di:0.60 */1037.48,-1,1.34,3,2,16,8,-1,3,6,16,4,2,0,0,0,0,0,
/* 79,fi:0.0115,di:0.59 */114.18,-1,1.34,13,4,4,4,-1,13,6,4,2,2,0,0,0,0,0,
/* 80,fi:0.0115,di:0.59 */305.95,-1,1.33,6,4,12,8,-1,6,6,12,4,2,0,0,0,0,0,
/* 81,fi:0.0110,di:0.59 */257.19,-1,1.33,2,4,10,8,-1,2,6,10,4,2,0,0,0,0,0,
/* 82,fi:0.0110,di:0.59 */180.93,-1,1.33,1,4,14,4,-1,1,6,14,2,2,0,0,0,0,0,
/* 83,fi:0.0105,di:0.59 */997.12,-1,1.33,2,2,16,8,-1,2,6,16,4,2,0,0,0,0,0,
/* 84,fi:0.0105,di:0.59 */133.20,-1,1.33,15,4,4,4,-1,15,6,4,2,2,0,0,0,0,0,
/* 85,fi:0.0105,di:0.59 */285.28,-1,1.33,1,4,12,8,-1,1,6,12,4,2,0,0,0,0,0,
/* 86,fi:0.0105,di:0.59 */277.11,-1,1.33,3,4,12,8,-1,3,6,12,4,2,0,0,0,0,0,
/* 87,fi:0.0100,di:0.59 */212.32,-1,1.33,10,4,8,8,-1,10,6,8,4,2,0,0,0,0,0,
/* 88,fi:0.0100,di:0.59 */284.01,-1,1.33,9,4,10,8,-1,9,6,10,4,2,0,0,0,0,0,
/* 89,fi:0.0100,di:0.59 */253.55,-1,1.33,7,4,6,8,-1,9,4,2,8,3,0,0,0,0,0,
/* 90,fi:0.0100,di:0.58 */476.46,-1,1.33,7,2,6,16,-1,9,2,2,16,3,0,0,0,0,0,
/* 91,fi:0.0100,di:0.58 */190.05,-1,1.32,16,2,2,12,-1,16,6,2,4,3,0,0,0,0,0,
/* 92,fi:0.0100,di:0.58 */494.72,-1,1.32,7,0,6,14,-1,9,0,2,14,3,0,0,0,0,0,
/* 93,fi:0.0100,di:0.58 */867.78,-1,1.32,4,2,14,8,-1,4,6,14,4,2,0,0,0,0,0,
/* 94,fi:0.0100,di:0.58 */205.01,-1,1.32,14,2,2,12,-1,14,6,2,4,3,0,0,0,0,0,
/* 95,fi:0.0100,di:0.58 */178.98,-1,1.32,9,4,10,4,-1,9,6,10,2,2,0,0,0,0,0,
/* 96,fi:0.0100,di:0.58 */258.80,-1,1.32,1,4,10,8,-1,1,6,10,4,2,0,0,0,0,0,
/* 97,fi:0.0100,di:0.57 */569.77,-1,1.32,14,3,4,12,-1,14,7,4,4,3,0,0,0,0,0,
/* 98,fi:0.0100,di:0.57 */66.45,-1,1.32,16,4,2,4,-1,16,6,2,2,2,0,0,0,0,0,
/* 99,fi:0.0095,di:0.56 */556.05,-1,1.32,14,0,4,12,-1,14,6,4,6,2,0,0,0,0,0,
/* 100,fi:0.0095,di:0.56 */1059.48,-1,1.32,0,2,18,8,-1,0,6,18,4,2,0,0,0,0,0,
/* 101,fi:0.0095,di:0.56 */1152.14,-1,1.32,13,0,6,18,-1,13,6,6,6,3,0,0,0,0,0,
/* 102,fi:0.0050,di:0.55 */53.89,-1,1.31,4,4,2,4,-1,4,6,2,2,2,0,0,0,0,0,
//--
/* 103,fi:0.0050,di:0.54 */580.34,-1,1.32,11,2,8,8,-1,11,6,8,4,2,0,0,0,0,0,
/* 104,fi:0.0050,di:0.54 */443.82,-1,1.32,15,0,2,18,-1,15,6,2,6,3,0,0,0,0,0,
/* 105,fi:0.0050,di:0.54 */933.38,-1,1.32,1,2,16,8,-1,1,6,16,4,2,0,0,0,0,0,
/* 106,fi:0.0050,di:0.54 */252.40,-1,1.32,5,4,14,4,-1,5,6,14,2,2,0,0,0,0,0,
/* 107,fi:0.0050,di:0.54 */405.04,-1,1.31,7,3,6,16,-1,9,3,2,16,3,0,0,0,0,0,
/* 108,fi:0.0050,di:0.54 */542.57,-1,1.31,7,0,6,16,-1,9,0,2,16,3,0,0,0,0,0,
/* 109,fi:0.0050,di:0.54 */985.44,-1,1.31,0,2,16,12,-1,0,6,16,4,3,0,0,0,0,0,
/* 110,fi:0.0050,di:0.54 */233.64,-1,1.31,3,4,10,8,-1,3,6,10,4,2,0,0,0,0,0,
/* 111,fi:0.0050,di:0.54 */317.93,-1,1.31,0,4,14,8,-1,0,6,14,4,2,0,0,0,0,0,
/* 112,fi:0.0050,di:0.54 */148.93,-1,1.31,0,4,14,4,-1,0,6,14,2,2,0,0,0,0,0,
/* 113,fi:0.0050,di:0.54 */188.14,-1,1.31,7,4,12,4,-1,7,6,12,2,2,0,0,0,0,0,
/* 114,fi:0.0050,di:0.54 */296.26,-1,1.31,15,3,2,12,-1,15,7,2,4,3,0,0,0,0,0,
/* 115,fi:0.0050,di:0.53 */39.45,-1,1.31,3,4,2,4,-1,3,6,2,2,2,0,0,0,0,0,
/* 116,fi:0.0050,di:0.53 */106.40,-1,1.31,11,4,6,4,-1,11,6,6,2,2,0,0,0,0,0,
/* 117,fi:0.0050,di:0.53 */156.35,-1,1.31,4,4,12,4,-1,4,6,12,2,2,0,0,0,0,0,
/* 118,fi:0.0050,di:0.53 */473.34,-1,1.31,12,2,6,8,-1,12,6,6,4,2,0,0,0,0,0,
/* 119,fi:0.0050,di:0.53 */279.78,-1,1.31,7,4,6,10,-1,9,4,2,10,3,0,0,0,0,0,
/* 120,fi:0.0050,di:0.53 */348.04,-1,1.31,7,4,6,14,-1,9,4,2,14,3,0,0,0,0,0,
/* 121,fi:0.0050,di:0.53 */137.04,-1,1.31,3,4,12,4,-1,3,6,12,2,2,0,0,0,0,0,
/* 122,fi:0.0050,di:0.51 */45.74,1,1.31,5,12,2,6,-1,5,14,2,2,3,0,0,0,0,0,
/* 123,fi:0.0050,di:0.51 */273.58,-1,1.35,2,4,12,8,-1,2,6,12,4,2,0,0,0,0,0,
/* 124,fi:0.0050,di:0.51 */185.74,-1,1.35,4,4,10,8,-1,4,6,10,4,2,0,0,0,0,0,
/* 125,fi:0.0050,di:0.51 */119.35,-1,1.34,1,4,12,4,-1,1,6,12,2,2,0,0,0,0,0,
/* 126,fi:0.0050,di:0.51 */150.41,-1,1.34,8,4,10,4,-1,8,6,10,2,2,0,0,0,0,0,
/* 127,fi:0.0050,di:0.51 */178.82,-1,1.33,6,4,12,4,-1,6,6,12,2,2,0,0,0,0,0,
/* 128,fi:0.0050,di:0.50 */195.18,-1,1.33,4,4,8,8,-1,4,6,8,4,2,0,0,0,0,0,
/* 129,fi:0.0050,di:0.50 */243.64,-1,1.32,3,4,8,8,-1,3,6,8,4,2,0,0,0,0,0,
/* 130,fi:0.0050,di:0.50 */185.70,-1,1.32,7,4,10,8,-1,7,6,10,4,2,0,0,0,0,0,
/* 131,fi:0.0050,di:0.50 */90.97,-1,1.32,1,4,6,4,-1,1,6,6,2,2,0,0,0,0,0,
/* 132,fi:0.0050,di:0.50 */135.21,-1,1.31,15,4,4,8,-1,15,6,4,4,2,0,0,0,0,0,
/* 133,fi:0.0050,di:0.50 */164.15,-1,1.31,5,4,12,4,-1,5,6,12,2,2,0,0,0,0,0,
/* 134,fi:0.0050,di:0.50 */120.90,-1,1.31,2,4,10,4,-1,2,6,10,2,2,0,0,0,0,0,
/* 135,fi:0.0050,di:0.47 */15.52,-1,1.31,9,6,2,6,-1,9,8,2,2,3,0,0,0,0,0,
/* 136,fi:0.0050,di:0.47 */141.00,-1,1.31,0,4,12,4,-1,0,6,12,2,2,0,0,0,0,0,
/* 137,fi:0.0050,di:0.47 */175.83,-1,1.30,1,4,8,8,-1,1,6,8,4,2,0,0,0,0,0,
/* 138,fi:0.0050,di:0.47 */128.26,-1,1.30,1,4,10,4,-1,1,6,10,2,2,0,0,0,0,0,
/* 139,fi:0.0050,di:0.47 */208.33,-1,1.30,11,4,8,8,-1,11,6,8,4,2,0,0,0,0,0,
/* 140,fi:0.0050,di:0.47 */74.15,-1,1.30,2,4,4,4,-1,2,6,4,2,2,0,0,0,0,0,
/* 141,fi:0.0050,di:0.47 */858.91,-1,1.29,0,2,16,8,-1,0,6,16,4,2,0,0,0,0,0,
/* 142,fi:0.0050,di:0.47 */123.17,-1,1.29,2,4,12,4,-1,2,6,12,2,2,0,0,0,0,0,
/* 143,fi:0.0050,di:0.47 */210.44,-1,1.29,2,4,8,8,-1,2,6,8,4,2,0,0,0,0,0,
/* 144,fi:0.0050,di:0.47 */161.68,-1,1.29,17,2,2,12,-1,17,6,2,4,3,0,0,0,0,0,
/* 145,fi:0.0050,di:0.47 */478.11,-1,1.29,15,3,4,12,-1,15,7,4,4,3,0,0,0,0,0,
/* 146,fi:0.0050,di:0.47 */234.12,-1,1.29,0,4,10,8,-1,0,6,10,4,2,0,0,0,0,0,
/* 147,fi:0.0050,di:0.47 */147.42,-1,1.29,5,4,10,8,-1,5,6,10,4,2,0,0,0,0,0,
/* 148,fi:0.0045,di:0.47 */94.27,-1,1.28,5,4,8,8,-1,5,6,8,4,2,0,0,0,0,0,
/* 149,fi:0.0045,di:0.47 */528.99,-1,1.28,10,2,8,8,-1,10,6,8,4,2,0,0,0,0,0,
/* 150,fi:0.0045,di:0.45 */3.30,-1,1.28,13,3,6,4,-1,13,5,6,2,2,0,0,0,0,0,
/* 151,fi:0.0045,di:0.45 */837.50,-1,1.28,3,2,14,8,-1,3,6,14,4,2,0,0,0,0,0,
/* 152,fi:0.0045,di:0.44 */128.94,-1,1.28,7,7,6,4,-1,9,7,2,4,3,0,0,0,0,0,
/* 153,fi:0.0045,di:0.44 */796.52,-1,1.28,2,2,14,12,-1,2,6,14,4,3,0,0,0,0,0,
/* 154,fi:0.0045,di:0.44 */103.38,-1,1.28,9,4,8,4,-1,9,6,8,2,2,0,0,0,0,0,
/* 155,fi:0.0045,di:0.44 */677.43,-1,1.28,7,2,12,12,-1,7,6,12,4,3,0,0,0,0,0,
/* 156,fi:0.0045,di:0.44 */134.81,-1,1.28,14,4,4,8,-1,14,6,4,4,2,0,0,0,0,0,
/* 157,fi:0.0045,di:0.44 */355.24,-1,1.28,9,2,8,12,-1,9,6,8,4,3,0,0,0,0,0,
/* 158,fi:0.0045,di:0.44 */638.49,-1,1.28,9,2,10,8,-1,9,6,10,4,2,0,0,0,0,0,
/* 159,fi:0.0025,di:0.41 */2.46,-1,1.28,12,9,4,2,-1,14,9,2,2,2,0,0,0,0,0,
//--
/* 160,fi:0.0025,di:0.41 */472.72,-1,1.28,14,1,4,16,-1,14,5,4,8,2,0,0,0,0,0,
/* 161,fi:0.0020,di:0.40 */-24.21,1,1.27,5,11,2,8,-1,5,13,2,4,2,0,0,0,0,0,
/* 162,fi:0.0020,di:0.40 */38.75,-1,1.30,8,6,4,6,-1,8,8,4,2,3,0,0,0,0,0,
/* 163,fi:0.0020,di:0.39 */-4.77,1,1.29,9,8,2,4,-1,9,10,2,2,2,0,0,0,0,0,
/* 164,fi:0.0015,di:0.39 */4.72,1,1.28,8,8,6,4,-1,8,10,6,2,2,0,0,0,0,0,
/* 165,fi:0.0015,di:0.38 */69.59,1,1.29,13,14,4,4,-1,13,14,2,2,2,15,16,2,2,2,
/* 166,fi:0.0015,di:0.37 */54.52,-1,1.32,17,4,2,8,-1,17,6,2,4,2,0,0,0,0,0,
/* 167,fi:0.0015,di:0.37 */127.70,-1,1.32,5,4,6,8,-1,5,6,6,4,2,0,0,0,0,0,
/* 168,fi:0.0015,di:0.37 */70.39,-1,1.32,6,4,8,8,-1,6,6,8,4,2,0,0,0,0,0,
/* 169,fi:0.0015,di:0.37 */10.86,1,1.31,9,8,6,4,-1,9,10,6,2,2,0,0,0,0,0,
/* 170,fi:0.0015,di:0.37 */43.33,-1,1.30,6,4,6,8,-1,6,6,6,4,2,0,0,0,0,0,
/* 171,fi:0.0015,di:0.37 */165.74,-1,1.30,4,4,6,8,-1,4,6,6,4,2,0,0,0,0,0,
/* 172,fi:0.0015,di:0.37 */106.61,-1,1.30,8,4,8,8,-1,8,6,8,4,2,0,0,0,0,0,
/* 173,fi:0.0015,di:0.37 */244.34,-1,1.30,7,5,6,10,-1,9,5,2,10,3,0,0,0,0,0,
/* 174,fi:0.0015,di:0.37 */145.93,-1,1.29,9,4,8,8,-1,9,6,8,4,2,0,0,0,0,0,
/* 175,fi:0.0015,di:0.36 */-11.83,1,1.29,4,7,4,2,-1,6,7,2,2,2,0,0,0,0,0,
/* 176,fi:0.0015,di:0.36 */86.80,-1,1.30,1,4,6,8,-1,1,6,6,4,2,0,0,0,0,0,
/* 177,fi:0.0015,di:0.36 */160.91,-1,1.30,6,4,10,8,-1,6,6,10,4,2,0,0,0,0,0,
/* 178,fi:0.0015,di:0.34 */-109.30,1,1.30,4,4,6,8,-1,6,4,2,8,3,0,0,0,0,0,
/* 179,fi:0.0015,di:0.33 */-44.25,1,1.29,4,8,6,2,-1,6,8,2,2,3,0,0,0,0,0,
/* 180,fi:0.0015,di:0.33 */79.31,-1,1.29,3,4,4,4,-1,3,6,4,2,2,0,0,0,0,0,
/* 181,fi:0.0015,di:0.33 */67.06,-1,1.29,16,4,2,8,-1,16,6,2,4,2,0,0,0,0,0,
/* 182,fi:0.0015,di:0.33 */-1.12,1,1.29,8,8,8,4,-1,8,10,8,2,2,0,0,0,0,0,
/* 183,fi:0.0015,di:0.32 */3.73,-1,1.29,12,8,4,4,-1,14,8,2,4,2,0,0,0,0,0,
/* 184,fi:0.0015,di:0.32 */-94.96,1,1.29,4,7,6,4,-1,6,7,2,4,3,0,0,0,0,0,
/* 185,fi:0.0015,di:0.32 */519.05,-1,1.29,8,2,10,12,-1,8,6,10,4,3,0,0,0,0,0,
/* 186,fi:0.0015,di:0.32 */84.56,-1,1.29,10,4,6,8,-1,10,6,6,4,2,0,0,0,0,0,
/* 187,fi:0.0015,di:0.32 */-144.66,1,1.28,4,2,6,10,-1,6,2,2,10,3,0,0,0,0,0,
/* 188,fi:0.0015,di:0.32 */590.92,-1,1.28,6,2,12,12,-1,6,6,12,4,3,0,0,0,0,0,
/* 189,fi:0.0015,di:0.32 */413.94,-1,1.28,16,0,2,18,-1,16,6,2,6,3,0,0,0,0,0,
/* 190,fi:0.0015,di:0.32 */87.72,-1,1.28,2,4,6,4,-1,2,6,6,2,2,0,0,0,0,0,
/* 191,fi:0.0015,di:0.32 */90.47,-1,1.28,11,4,6,8,-1,11,6,6,4,2,0,0,0,0,0,
/* 192,fi:0.0015,di:0.32 */8.05,1,1.28,9,8,4,4,-1,9,10,4,2,2,0,0,0,0,0,
/* 193,fi:0.0015,di:0.32 */191.82,-1,1.28,15,3,2,8,-1,15,7,2,4,2,0,0,0,0,0,
/* 194,fi:0.0015,di:0.32 */121.27,-1,1.28,1,4,8,4,-1,1,6,8,2,2,0,0,0,0,0,
/* 195,fi:0.0010,di:0.31 */185.74,1,1.28,5,5,6,6,-1,7,5,2,6,3,0,0,0,0,0,
//--
/* 196,fi:0.0010,di:0.31 */-3.58,-1,1.28,15,3,4,4,-1,15,5,4,2,2,0,0,0,0,0,
/* 197,fi:0.0010,di:0.31 */103.55,-1,1.28,15,1,2,8,-1,15,5,2,4,2,0,0,0,0,0,
/* 198,fi:0.0010,di:0.31 */397.91,-1,1.28,13,2,4,8,-1,13,6,4,4,2,0,0,0,0,0,
/* 199,fi:0.0010,di:0.31 */150.54,-1,1.28,3,4,6,8,-1,3,6,6,4,2,0,0,0,0,0,
/* 200,fi:0.0005,di:0.30 */5.13,1,1.28,5,8,10,4,-1,5,10,10,2,2,0,0,0,0,0,
//--
/* 201,fi:0.0005,di:0.30 */234.33,-1,1.28,16,3,2,12,-1,16,7,2,4,3,0,0,0,0,0,
/* 202,fi:0.0005,di:0.30 */191.29,-1,1.28,15,1,4,8,-1,15,5,4,4,2,0,0,0,0,0,
/* 203,fi:0.0005,di:0.30 */283.73,-1,1.28,7,5,6,12,-1,9,5,2,12,3,0,0,0,0,0,
/* 204,fi:0.0005,di:0.30 */243.69,-1,1.27,15,1,2,16,-1,15,5,2,8,2,0,0,0,0,0,
/* 205,fi:0.0005,di:0.30 */112.18,1,1.27,5,6,6,4,-1,7,6,2,4,3,0,0,0,0,0,
/* 206,fi:0.0005,di:0.30 */7.54,1,1.27,7,8,8,4,-1,7,10,8,2,2,0,0,0,0,0,
/* 207,fi:0.0005,di:0.30 */73.59,-1,1.27,16,1,2,8,-1,16,5,2,4,2,0,0,0,0,0,
/* 208,fi:0.0005,di:0.30 */-15.72,1,1.27,4,6,4,6,-1,6,6,2,6,2,0,0,0,0,0,
/* 209,fi:0.0005,di:0.30 */711.67,-1,1.27,13,3,6,12,-1,13,7,6,4,3,0,0,0,0,0,
/* 210,fi:0.0005,di:0.30 */64.57,-1,1.27,15,4,2,8,-1,15,6,2,4,2,0,0,0,0,0,
/* 211,fi:0.0005,di:0.30 */317.52,-1,1.27,13,1,6,8,-1,13,5,6,4,2,0,0,0,0,0,
/* 212,fi:0.0005,di:0.30 */204.32,-1,1.27,4,2,2,12,-1,4,6,2,4,3,0,0,0,0,0,
/* 213,fi:0.0005,di:0.30 */587.38,-1,1.27,5,2,12,12,-1,5,6,12,4,3,0,0,0,0,0,
/* 214,fi:0.0005,di:0.30 */190.02,-1,1.27,13,4,6,8,-1,13,6,6,4,2,0,0,0,0,0,
/* 215,fi:0.0005,di:0.30 */124.66,-1,1.27,0,4,10,4,-1,0,6,10,2,2,0,0,0,0,0,
/* 216,fi:0.0005,di:0.30 */11.78,-1,1.27,9,6,4,6,-1,9,8,4,2,3,0,0,0,0,0,
/* 217,fi:0.0005,di:0.28 */3.44,-1,1.27,11,9,8,2,-1,15,9,4,2,2,0,0,0,0,0,
/* 218,fi:0.0005,di:0.28 */-11.91,1,1.27,4,8,8,4,-1,4,10,8,2,2,0,0,0,0,0,
/* 219,fi:0.0005,di:0.28 */6.63,-1,1.27,11,8,8,4,-1,15,8,4,4,2,0,0,0,0,0,
/* 220,fi:0.0005,di:0.28 */17.25,1,1.27,9,8,8,4,-1,9,10,8,2,2,0,0,0,0,0,
/* 221,fi:0.0005,di:0.28 */103.11,-1,1.27,3,4,8,4,-1,3,6,8,2,2,0,0,0,0,0,
/* 222,fi:0.0005,di:0.28 */1207.72,-1,1.27,3,3,16,12,-1,3,7,16,4,3,0,0,0,0,0,
/* 223,fi:0.0005,di:0.28 */49.71,-1,1.27,14,4,2,8,-1,14,6,2,4,2,0,0,0,0,0,
/* 224,fi:0.0005,di:0.28 */-10.68,1,1.26,6,8,6,4,-1,6,10,6,2,2,0,0,0,0,0,
/* 225,fi:0.0005,di:0.28 */447.29,-1,1.26,2,2,10,12,-1,2,6,10,4,3,0,0,0,0,0,
/* 226,fi:0.0005,di:0.27 */39.38,-1,1.26,17,4,2,4,-1,17,6,2,2,2,0,0,0,0,0,
/* 227,fi:0.0005,di:0.27 */84.97,-1,1.26,4,6,8,6,-1,4,8,8,2,3,0,0,0,0,0,
/* 228,fi:0.0005,di:0.27 */-108.24,1,1.26,4,6,6,6,-1,6,6,2,6,3,0,0,0,0,0,
/* 229,fi:0.0005,di:0.27 */-11.11,1,1.26,5,8,8,4,-1,5,10,8,2,2,0,0,0,0,0,
/* 230,fi:0.0005,di:0.27 */107.50,-1,1.26,7,4,10,4,-1,7,6,10,2,2,0,0,0,0,0,
/* 231,fi:0.0005,di:0.27 */-10.88,1,1.26,4,8,10,4,-1,4,10,10,2,2,0,0,0,0,0,
/* 232,fi:0.0005,di:0.27 */125.10,-1,1.26,12,4,6,8,-1,12,6,6,4,2,0,0,0,0,0,
/* 233,fi:0.0005,di:0.27 */36.16,-1,1.26,7,6,4,6,-1,7,8,4,2,3,0,0,0,0,0,
/* 234,fi:0.0005,di:0.27 */-49.51,1,1.26,2,8,16,4,-1,6,8,8,4,2,0,0,0,0,0,
/* 235,fi:0.0005,di:0.27 */618.40,-1,1.26,4,2,12,12,-1,4,6,12,4,3,0,0,0,0,0,
/* 236,fi:0.0005,di:0.27 */61.58,-1,1.26,7,4,8,8,-1,7,6,8,4,2,0,0,0,0,0,
/* 237,fi:0.0005,di:0.27 */87.01,-1,1.26,3,4,4,8,-1,3,6,4,4,2,0,0,0,0,0,
/* 238,fi:0.0005,di:0.27 */-8.30,1,1.26,6,8,8,4,-1,6,10,8,2,2,0,0,0,0,0,
/* 239,fi:0.0005,di:0.26 */7.60,-1,1.26,0,7,8,4,-1,2,7,4,4,2,0,0,0,0,0,
/* 240,fi:0.0005,di:0.26 */756.78,-1,1.26,2,2,14,8,-1,2,6,14,4,2,0,0,0,0,0,
/* 241,fi:0.0005,di:0.26 */338.77,-1,1.26,2,3,4,12,-1,2,7,4,4,3,0,0,0,0,0,
/* 242,fi:0.0005,di:0.26 */-124.14,1,1.26,4,3,6,10,-1,6,3,2,10,3,0,0,0,0,0,
/* 243,fi:0.0005,di:0.26 */71.05,-1,1.25,3,4,10,4,-1,3,6,10,2,2,0,0,0,0,0,
/* 244,fi:0.0005,di:0.26 */102.83,-1,1.25,13,4,4,8,-1,13,6,4,4,2,0,0,0,0,0,
/* 245,fi:0.0005,di:0.26 */-19.62,1,1.25,4,6,4,4,-1,6,6,2,4,2,0,0,0,0,0,
/* 246,fi:0.0005,di:0.25 */128.27,-1,1.25,7,5,6,4,-1,9,5,2,4,3,0,0,0,0,0,
/* 247,fi:0.0005,di:0.25 */201.91,-1,1.25,7,6,6,10,-1,9,6,2,10,3,0,0,0,0,0,
/* 248,fi:0.0005,di:0.25 */-112.61,1,1.25,4,5,6,6,-1,6,5,2,6,3,0,0,0,0,0,
/* 249,fi:0.0005,di:0.25 */28.44,-1,1.25,8,6,6,6,-1,8,8,6,2,3,0,0,0,0,0,
/* 250,fi:0.0005,di:0.25 */-13.14,-1,1.25,15,3,2,4,-1,15,5,2,2,2,0,0,0,0,0,
/* 251,fi:0.0005,di:0.25 */-14.19,-1,1.26,10,6,2,6,-1,10,8,2,2,3,0,0,0,0,0,
/* 252,fi:0.0005,di:0.25 */16.74,1,1.26,3,7,8,4,-1,5,7,4,4,2,0,0,0,0,0,
/* 253,fi:0.0005,di:0.25 */2.13,1,1.25,3,8,8,2,-1,5,8,4,2,2,0,0,0,0,0,
/* 254,fi:0.0005,di:0.24 */-0.67,-1,1.25,9,9,4,4,-1,9,9,2,2,2,11,11,2,2,2,
/* 255,fi:0.0005,di:0.24 */209.20,-1,1.26,14,1,4,8,-1,14,5,4,4,2,0,0,0,0,0,
/* 256,fi:0.0005,di:0.24 */350.64,-1,1.25,14,3,4,8,-1,14,7,4,4,2,0,0,0,0,0,
/* 257,fi:0.0005,di:0.24 */-22.05,-1,1.25,14,3,4,4,-1,14,5,4,2,2,0,0,0,0,0,
/* 258,fi:0.0005,di:0.24 */15.57,-1,1.25,10,9,8,2,-1,14,9,4,2,2,0,0,0,0,0,
/* 259,fi:0.0005,di:0.24 */1169.39,-1,1.25,2,3,16,12,-1,2,7,16,4,3,0,0,0,0,0,
/* 260,fi:0.0005,di:0.24 */796.78,-1,1.25,5,2,14,8,-1,5,6,14,4,2,0,0,0,0,0,
/* 261,fi:0.0005,di:0.24 */-37.63,-1,1.25,15,4,4,6,-1,15,6,4,2,3,0,0,0,0,0,
/* 262,fi:0.0005,di:0.24 */223.49,-1,1.25,3,3,2,12,-1,3,7,2,4,3,0,0,0,0,0,
/* 263,fi:0.0005,di:0.24 */113.69,-1,1.25,0,4,8,4,-1,0,6,8,2,2,0,0,0,0,0,
/* 264,fi:0.0005,di:0.24 */59.69,-1,1.25,10,8,8,4,-1,14,8,4,4,2,0,0,0,0,0,
/* 265,fi:0.0005,di:0.24 */92.13,-1,1.25,2,4,8,4,-1,2,6,8,2,2,0,0,0,0,0,
/* 266,fi:0.0005,di:0.24 */1363.38,-1,1.25,1,3,18,12,-1,1,7,18,4,3,0,0,0,0,0,
/* 267,fi:0.0005,di:0.24 */0.15,1,1.25,8,8,4,4,-1,8,10,4,2,2,0,0,0,0,0,
/* 268,fi:0.0005,di:0.24 */24.10,1,1.25,9,8,10,4,-1,9,10,10,2,2,0,0,0,0,0,
/* 269,fi:0.0005,di:0.24 */269.24,-1,1.25,14,3,2,12,-1,14,7,2,4,3,0,0,0,0,0,
/* 270,fi:0.0005,di:0.24 */293.44,1,1.25,5,4,6,8,-1,7,4,2,8,3,0,0,0,0,0,
/* 271,fi:0.0005,di:0.24 */276.26,-1,1.25,1,2,6,8,-1,1,6,6,4,2,0,0,0,0,0,
/* 272,fi:0.0005,di:0.24 */58.23,-1,1.25,10,4,6,4,-1,10,6,6,2,2,0,0,0,0,0,
/* 273,fi:0.0005,di:0.24 */-115.18,1,1.25,4,3,6,8,-1,6,3,2,8,3,0,0,0,0,0,
/* 274,fi:0.0005,di:0.24 */169.59,1,1.24,5,6,6,6,-1,7,6,2,6,3,0,0,0,0,0,
/* 275,fi:0.0005,di:0.24 */241.25,1,1.24,5,5,6,8,-1,7,5,2,8,3,0,0,0,0,0,
/* 276,fi:0.0005,di:0.24 */-30.11,-1,1.24,11,5,8,6,-1,15,5,4,6,2,0,0,0,0,0,
/* 277,fi:0.0005,di:0.23 */11.84,-1,1.24,11,7,8,4,-1,15,7,4,4,2,0,0,0,0,0,
/* 278,fi:0.0005,di:0.23 */-177.37,1,1.24,4,5,6,12,-1,6,5,2,12,3,0,0,0,0,0,
/* 279,fi:0.0005,di:0.23 */4.37,-1,1.24,8,4,4,8,-1,8,6,4,4,2,0,0,0,0,0,
/* 280,fi:0.0005,di:0.23 */-26.05,1,1.24,5,8,6,4,-1,5,10,6,2,2,0,0,0,0,0,
/* 281,fi:0.0005,di:0.23 */13.72,-1,1.24,11,6,8,6,-1,15,6,4,6,2,0,0,0,0,0,
/* 282,fi:0.0005,di:0.23 */145.59,-1,1.24,7,6,6,6,-1,9,6,2,6,3,0,0,0,0,0,
/* 283,fi:0.0005,di:0.23 */112.47,-1,1.24,2,4,6,8,-1,2,6,6,4,2,0,0,0,0,0,
/* 284,fi:0.0005,di:0.23 */-40.67,-1,1.24,7,8,4,2,-1,9,8,2,2,2,0,0,0,0,0,
/* 285,fi:0.0005,di:0.23 */10.98,-1,1.26,0,8,8,2,-1,2,8,4,2,2,0,0,0,0,0,
/* 286,fi:0.0005,di:0.23 */-25.67,-1,1.25,13,3,4,4,-1,13,5,4,2,2,0,0,0,0,0,
/* 287,fi:0.0005,di:0.23 */-2.54,-1,1.25,5,3,12,4,-1,5,5,12,2,2,0,0,0,0,0,
/* 288,fi:0.0005,di:0.23 */667.15,-1,1.25,13,1,6,16,-1,13,5,6,8,2,0,0,0,0,0,
/* 289,fi:0.0005,di:0.23 */336.42,-1,1.25,1,2,6,12,-1,1,6,6,4,3,0,0,0,0,0,
/* 290,fi:0.0005,di:0.23 */61.77,-1,1.25,2,4,4,8,-1,2,6,4,4,2,0,0,0,0,0,
/* 291,fi:0.0005,di:0.23 */-0.84,1,1.25,8,8,10,4,-1,8,10,10,2,2,0,0,0,0,0,
/* 292,fi:0.0005,di:0.23 */60.52,-1,1.25,1,4,4,4,-1,1,6,4,2,2,0,0,0,0,0,
/* 293,fi:0.0005,di:0.23 */263.57,-1,1.25,17,0,2,18,-1,17,6,2,6,3,0,0,0,0,0,
/* 294,fi:0.0005,di:0.23 */320.25,-1,1.24,11,2,6,12,-1,11,6,6,4,3,0,0,0,0,0,
/* 295,fi:0.0005,di:0.23 */36.76,-1,1.24,10,6,8,6,-1,14,6,4,6,2,0,0,0,0,0,
/* 296,fi:0.0005,di:0.23 */-17.53,-1,1.24,16,3,2,4,-1,16,5,2,2,2,0,0,0,0,0,
/* 297,fi:0.0005,di:0.22 */221.10,-1,1.24,4,3,2,12,-1,4,7,2,4,3,0,0,0,0,0,
/* 298,fi:0.0005,di:0.22 */-56.87,1,1.24,4,8,6,8,-1,6,8,2,8,3,0,0,0,0,0,
/* 299,fi:0.0005,di:0.22 */227.91,-1,1.24,2,2,4,8,-1,2,6,4,4,2,0,0,0,0,0,
/* 300,fi:0.0005,di:0.22 */13.75,-1,1.24,4,3,14,4,-1,4,5,14,2,2,0,0,0,0,0,
/* 301,fi:0.0005,di:0.22 */393.07,-1,1.24,15,1,4,16,-1,15,5,4,8,2,0,0,0,0,0,
/* 302,fi:0.0005,di:0.22 */76.94,1,1.24,2,6,16,6,-1,6,6,8,6,2,0,0,0,0,0,
/* 303,fi:0.0005,di:0.22 */100.61,-1,1.24,14,1,2,8,-1,14,5,2,4,2,0,0,0,0,0,
/* 304,fi:0.0005,di:0.22 */-302.88,1,1.24,2,0,16,8,-1,2,2,16,4,2,0,0,0,0,0,
/* 305,fi:0.0005,di:0.22 */144.75,-1,1.24,16,3,2,8,-1,16,7,2,4,2,0,0,0,0,0,
/* 306,fi:0.0005,di:0.22 */398.57,-1,1.24,11,2,6,8,-1,11,6,6,4,2,0,0,0,0,0,
/* 307,fi:0.0005,di:0.22 */5.43,-1,1.24,9,3,10,4,-1,9,5,10,2,2,0,0,0,0,0,
/* 308,fi:0.0005,di:0.22 */104.53,-1,1.24,5,2,2,12,-1,5,6,2,4,3,0,0,0,0,0,
/* 309,fi:0.0005,di:0.22 */528.25,-1,1.24,8,2,10,8,-1,8,6,10,4,2,0,0,0,0,0,
/* 310,fi:0.0005,di:0.22 */102.15,-1,1.24,1,5,18,6,-1,1,7,18,2,3,0,0,0,0,0,
/* 311,fi:0.0005,di:0.22 */2.34,1,1.24,4,7,4,4,-1,6,7,2,4,2,0,0,0,0,0,
/* 312,fi:0.0005,di:0.21 */-24.99,-1,1.24,13,6,4,10,-1,15,6,2,10,2,0,0,0,0,0,
/* 313,fi:0.0005,di:0.21 */633.54,-1,1.23,1,2,14,8,-1,1,6,14,4,2,0,0,0,0,0,
/* 314,fi:0.0005,di:0.21 */292.51,-1,1.23,3,2,4,12,-1,3,6,4,4,3,0,0,0,0,0,
/* 315,fi:0.0000,di:0.21 */-41.95,-1,1.23,2,14,4,4,-1,2,14,2,2,2,4,16,2,2,2,
//--
  };
  static CvMat haarclassifier_face20 =
      cvMat(316,18,CV_32F,haarclassifier_face20_data);
  return &haarclassifier_face20;
}
