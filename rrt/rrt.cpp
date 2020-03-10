#include "stdio.h"
#include <stdlib.h>
#include <vector>
#include <math.h>
#include "opencv2/core/core_c.h"
#include "opencv2/core/core.hpp"
#include "opencv2/flann/miniflann.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/highgui/highgui.hpp"


using namespace cv;
using namespace std;

typedef struct
{
  int x;
  int y;
}coordi;

struct Nodegraph {
    vector<Nodegraph *> children;
    Nodegraph *parent;
    coordi position;
};

Nodegraph start_node;
Nodegraph end_node;
Mat img;
Nodegraph* nodes[500000];
long int totnodes = 0;
int reached = 0;
int step_size = 10;
long int iter = 0;

void init()
{
for(int i=0;i<img.rows;i++)
{
for(int j=0;j<img.cols;j++)
{
if((img.at<Vec3b>(i,j)[1]>120)&&(img.at<Vec3b>(i,j)[0]<50))
{
start_node.position.x = j;
start_node.position.y = i;
}
if((img.at<Vec3b>(i,j)[2]>120)&&(img.at<Vec3b>(i,j)[0]<50))
{
end_node.position.x = j;
end_node.position.y = i;
}
}
}
start_node.parent = NULL;
nodes[totnodes++] = &start_node;
srand(time(NULL));
}
float node_dist(coordi p, coordi q)
{
  coordi v;
  v.x = p.x - q.x;
  v.y = p.y - q.y;
  return sqrt(pow(v.x, 2) + pow(v.y, 2));
}
int near_node(Nodegraph rnode)
{
  float min_dist = 999.0, dist= node_dist(start_node.position, rnode.position);
  int lnode = 0, i = 0;
  for(i=0; i<totnodes; i++)
  {
    dist = node_dist(nodes[i]->position, rnode.position);
    if(dist<min_dist)
    {
      min_dist = dist;
      lnode = i;
    }
  }
  return lnode;
}
coordi stepping(coordi nnode, coordi rnode)
{
  coordi interm, step;
  float magn = 0.0, x = 0.0, y = 0.0;
  interm.x = rnode.x - nnode.x;
  interm.y = rnode.y - nnode.y;
  magn = sqrt((interm.x)*(interm.x) + (interm.y)*(interm.y));
  x = (float)(interm.x / magn);
  y = (float)(interm.y / magn);
  step.x = (int)(nnode.x + step_size*x);
  step.y = (int)(nnode.y + step_size*y);
  return step;
}
int check_validity_1(coordi p, coordi q)
{
  coordi large, small;
  int i = 0, j1 = 0, j2 = 0;
  double slope;
  if(q.x<p.x)
  {
    small = q;
    large = p;
  }
  else
  {
    small = p;
    large = q;
  }
  if(large.x == small.x)
    return 0;
  slope = ((double)large.y - small.y)/((double)large.x - small.x);
  for(i=small.x+1; i<large.x; i++)
  {
    j1 = (int)((i*slope) - (small.x)*(slope) + small.y);
    j2 = j1 + 1;
    if((i<0) || (i>400) || (j1<0) || (j1>400) || (j2<0) || (j2>400))
      continue;
    if(((int)img.at<Vec3b>(i, j1)[0] >100) && ((int)img.at<Vec3b>(i, j1)[1] >100) && ((int)img.at<Vec3b>(i, j1)[2] > 100))
     return 0;
     if(((int)img.at<Vec3b>(i, j2)[0] >100) && ((int)img.at<Vec3b>(i, j2)[1] > 100) && ((int)img.at<Vec3b>(i, j2)[2] > 100))
      return 0;
  }
  return 1;
}
int check_validity_2(coordi p, coordi q)
{
  coordi large, small;
  int i = 0, j1 = 0, j2 = 0;
  double slope;
  if(q.y<p.y)
  {
    small = q;
    large = p;
  }
  else
  {
    small = p;
    large = q;
  }
  if(large.x == small.x)
    return 0;
  slope = ((double)large.y - small.y)/((double)large.x - small.x);
  for(i=small.y+1; i<large.y; i++)
  {
    j1 = (int)(((i - small.y)/slope) + small.x);
    j2 = j1 + 1;
    if((i<0) || (i>400) || (j1<0) || (j1>400) || (j2<0) || (j2>400))
      continue;
    if(((int)img.at<Vec3b>(j1, i)[0] >100) && ((int)img.at<Vec3b>(j1, i)[1] >100) && ((int)img.at<Vec3b>(j1, i)[2] > 100))
     return 0;
     if(((int)img.at<Vec3b>(j2, i)[0] >100) && ((int)img.at<Vec3b>(j2, i)[1] > 100) && ((int)img.at<Vec3b>(j2, i)[2] > 100))
      return 0;
  }
  return 1;
}
void draw_path()
{
  Nodegraph up, down;
  int breaking = 0;
  down = end_node;
  up = *(end_node.parent);
  while(1)
  {

    line(img, Point(up.position.y, up.position.x), Point(down.position.y, down.position.x), Scalar(0, 255, 0), 2, 8);
    if(up.parent == NULL)
      break;

    up = *(up.parent);
    down = *(down.parent);
  }
}

void rrt()
{
  int flag1 = 0, index = 0, flag2 = 0;
    Nodegraph* rnode = new Nodegraph;
    Nodegraph* stepnode = new Nodegraph;
    (rnode->position).x = rand() % 4000 + 1;
    (rnode->position).y = rand() % 4000 + 1;
    index = near_node(*rnode);

    if((node_dist(rnode->position, nodes[index]->position)) < step_size)
{

      return;
        }

    else
{
      stepnode->position = stepping(nodes[index]->position, rnode->position);
}
if((stepnode->position.x>img.cols)||(stepnode->position.y>img.rows))
{
return;
}
    flag1 = check_validity_1(nodes[index]->position, stepnode->position);
    flag2 = check_validity_2(nodes[index]->position, stepnode->position);
    if((flag1 == 1) && (flag2 == 1))
    {
      nodes[totnodes++] = stepnode;
      stepnode->parent = nodes[index];
      (nodes[index]->children).push_back(stepnode);
      line(img, Point((stepnode->position).y, (stepnode->position).x), Point(nodes[index]->position.y, nodes[index]->position.x), Scalar(0, 255, 255), 2, 8);

      for(int i=stepnode->position.x - 2; i < stepnode->position.x + 2; i++)
      {
        for(int j=stepnode->position.y - 2; j < stepnode->position.y + 2; j++)
        {
          if((i<0) || (i>4000) || (j<0) || (j>4000))
            continue;

          img.at<Vec3b>(i, j)[0] = 0;
          img.at<Vec3b>(i, j)[1] = 255;
          img.at<Vec3b>(i, j)[2] = 0;
        }
      }

      if((check_validity_1(stepnode->position, end_node.position)) && (check_validity_2(stepnode->position, end_node.position)) && (node_dist(stepnode->position,end_node.position) < step_size))
      {
cout<<"NO"<<"\n";
reached = 1;
        nodes[totnodes++] = &end_node;
        end_node.parent = stepnode;
        (nodes[totnodes-1]->children).push_back(&end_node);
        draw_path();
      }
    }
    iter++;
}

int main()
{
  img = imread("/home/adarshares/Desktop/rrt/IMG-20200309-WA0001.jpg", CV_LOAD_IMAGE_COLOR);
  init();
  cout<<start_node.position.x<<" "<<start_node.position.y<<" "<<end_node.position.x<<" "<<end_node.position.y<<"\n";
  while((reached ==0))
  {
    rrt();
  }

  namedWindow( "window", WINDOW_NORMAL );
  imshow("window", img);
  imwrite("output.png", img);
  waitKey();
  return 0;
}


