/**
 * @file   mainwindow.h
 * @author Liangfu Chen <liangfu.chen@nlpr.ia.ac.cn>
 * @date   Thu Jan 23 20:35:25 2014
 * 
 * @brief  
 * 
 * 
 */

#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <stdio.h>
#include <assert.h>

#include "ui_compvis.h"
#include <QtGui>
//#include <QtOpenGL>

#include "compvis.h"
#include "canvas.h"
#include "highgui.h"
// #include "meshviewer.h"
#include "cvstageddetecthaar.h"

class MainWindow : public QMainWindow,public Ui::MainWindow
{
  Q_OBJECT
  
private:
  CvStagedDetectorHaar m_facedetector;
public:

  void openFile(QString fname)
  {
  	if (!fname.isEmpty())
  	{
	  const char * fnamestr=fname.toAscii();
  	  char suffix[1024];
  	  cvGetFileSuffix(fnamestr,suffix);
  	  if ((!strncmp(suffix+1,"jpg",3))||(!strncmp(suffix+1,"png",3))){
  		// display image
		test_facedetector_pic(fnamestr);
		// IplImage * img = cvLoadImage(fnamestr,1);
		// CvMat img_stub;
		// CvMat * mat = cvGetMat(img,&img_stub);
		// widget->display(mat);
  	  }else if ((!strncmp(suffix+1,"avi",3))||(!strncmp(suffix+1,"mp4",3))){
  	  }else{
  		assert(false);
  	  }
  	}
  }
  
private:
  int m_initialized;
  int initialized(){return m_initialized;}
  void initialize()
  {
	m_initialized=1;
  }

  void test_facedetector_pic(const char * fname)
  {
	int i;
	IplImage * img = cvLoadImage(fname,1);
	if (!img){
	  QMessageBox::warning(this, tr("Warning"),tr("Fail to load image file!"));
	}else{
	  CvMat * gray = cvCreateMat(img->height,img->width,CV_8U);
	  cvCvtColor(img,gray,CV_BGR2GRAY);
	  CvRect roiarr[50];
	  int nfaces=m_facedetector.detect(gray,roiarr);
	  CvMat img_stub;
	  CvMat * mat = cvGetMat(img,&img_stub);
	  for (i=0;i<nfaces;i++)
	  {
		cvRectangle(mat,
					cvPoint(roiarr[i].x,roiarr[i].y),
					cvPoint(roiarr[i].x+roiarr[i].width,
							roiarr[i].y+roiarr[i].height),CV_RED,3);
	  }
	  widget->display(mat);
	  cvReleaseMat(&gray);
	}
  }
  
protected:
  void dragEnterEvent(QDragEnterEvent *e)
  {
    if (e->mimeData()->hasUrls()) {
	  e->acceptProposedAction();
	}
  }

  void dropEvent(QDropEvent *e)
  {
	int i;
    for (i=0;i<e->mimeData()->urls().count();i++) {
	  const QUrl url=e->mimeData()->urls()[i];
	  const QString fname = url.toLocalFile();
	  fprintf(stderr,"drop file: %s\n",qPrintable(fname));
	  openFile(fname);
	}
  }

private slots:
  void on_pushButton_load_clicked()
  {
	QString fname =
	  QFileDialog::getOpenFileName(this,"select file",".",
								   "Images files [*.png,*.jpg] (*.png *.jpg);;"
								   "Video files [*.avi,*.mp4] (*.avi,*.mp4);;"
								   "All files (*.*)");
	openFile(fname);
  }

  void on_pushButton_camera_clicked()
  {
  }
  
  void on_pushButton_start_clicked()
  {
  }
  
  // void on_actionFirst_triggered()
  // {
  // 	if (graphicsView){
  // 	  graphicsView->first();
  // 	  setWindowTitle(graphicsView->currentFileName());
  // 	}
  // }
  
  // void on_actionLast_triggered()
  // {
  // 	if (graphicsView){
  // 	  graphicsView->last();
  // 	  setWindowTitle(graphicsView->currentFileName());
  // 	}
  // }
  
  // void on_actionPrevious_triggered()
  // {
  // 	if (graphicsView){
  // 	  graphicsView->previous();
  // 	  setWindowTitle(graphicsView->currentFileName());
  // 	}
  // }
  
  // void on_actionNext_triggered()
  // {
  // 	if (graphicsView){
  // 	  graphicsView->next();
  // 	  setWindowTitle(graphicsView->currentFileName());
  // 	}
  // }
  
  // void on_actionAbout_triggered()
  // {
  // 	QMessageBox::about(this, tr("About CompVis"),
  // 					   tr("<b>CompVis</b> is an application designed for "
  // 						  "vision-base media content manipulations."));
  // }

public:
  MainWindow(QWidget * parent=0):
	QMainWindow(parent),m_initialized(0)
  {
	setupUi(this);
	window()->layout()->setSizeConstraint( QLayout::SetFixedSize );
  }
};

#endif // __MAIN_WINDOW_H__