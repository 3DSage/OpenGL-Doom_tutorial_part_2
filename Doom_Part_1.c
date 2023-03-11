#include <math.h>
#include <stdio.h>
#include <GL/glut.h> 

#define res        1                        //0=160x120 1=360x240 4=640x480
#define SW         160*res                  //screen width
#define SH         120*res                  //screen height
#define SW2        (SW/2)                   //half of screen width
#define SH2        (SH/2)                   //half of screen height
#define pixelScale 4/res                    //OpenGL pixel scale
#define GLSW       (SW*pixelScale)          //OpenGL window width
#define GLSH       (SH*pixelScale)          //OpenGL window height
#define numSect  4                          //number of sectors
#define numWall 16                          //number of walls
//------------------------------------------------------------------------------
typedef struct 
{
 int fr1,fr2;           //frame 1 frame 2, to create constant frame rate
}time; time T;

typedef struct 
{
 int w,s,a,d;           //move up, down, left, rigth
 int sl,sr;             //strafe left, right 
 int m;                 //move up, down, look up, down
}keys; keys K;

typedef struct 
{
 float cos[360];        //Save sin cos in values 0-360 degrees 
 float sin[360];
}math; math M;

typedef struct 
{
 int x,y,z;             //player position. Z is up
 int a;                 //player angle of rotation left right
 int l;                 //variable to look up and down
}player; player P;

typedef struct 
{
 int x1,y1;             //bottom line point 1
 int x2,y2;             //bottom line point 2
 int c;                 //wall color
}walls; walls W[30];

typedef struct 
{
 int ws,we;             //wall number start and end
 int z1,z2;             //height of bottom and top 
 int d;                 //add y distances to sort drawing order
 int c1,c2;             //bottom and top color
 int surf[SW];          //to hold points for surfaces
 int surface;           //is there a surfaces to draw
}sectors; sectors S[30];

//------------------------------------------------------------------------------

void pixel(int x,int y, int c)                  //draw a pixel at x/y with rgb
{int rgb[3];
 if(c==0){ rgb[0]=255; rgb[1]=255; rgb[2]=  0;} //Yellow	
 if(c==1){ rgb[0]=160; rgb[1]=160; rgb[2]=  0;} //Yellow darker	
 if(c==2){ rgb[0]=  0; rgb[1]=255; rgb[2]=  0;} //Green	
 if(c==3){ rgb[0]=  0; rgb[1]=160; rgb[2]=  0;} //Green darker	
 if(c==4){ rgb[0]=  0; rgb[1]=255; rgb[2]=255;} //Cyan	
 if(c==5){ rgb[0]=  0; rgb[1]=160; rgb[2]=160;} //Cyan darker
 if(c==6){ rgb[0]=160; rgb[1]=100; rgb[2]=  0;} //brown	
 if(c==7){ rgb[0]=110; rgb[1]= 50; rgb[2]=  0;} //brown darker
 if(c==8){ rgb[0]=  0; rgb[1]= 60; rgb[2]=130;} //background 
 glColor3ub(rgb[0],rgb[1],rgb[2]); 
 glBegin(GL_POINTS);
 glVertex2i(x*pixelScale+2,y*pixelScale+2);
 glEnd();
}

void movePlayer()
{
 //move up, down, left, right
 if(K.a ==1 && K.m==0){ P.a-=4; if(P.a<  0){ P.a+=360;}}  
 if(K.d ==1 && K.m==0){ P.a+=4; if(P.a>359){ P.a-=360;}}
 int dx=M.sin[P.a]*10.0; 
 int dy=M.cos[P.a]*10.0; 
 if(K.w ==1 && K.m==0){ P.x+=dx; P.y+=dy;}
 if(K.s ==1 && K.m==0){ P.x-=dx; P.y-=dy;}
 //strafe left, right
 if(K.sr==1){ P.x+=dy; P.y-=dx;}
 if(K.sl==1){ P.x-=dy; P.y+=dx;}
 //move up, down, look up, look down
 if(K.a==1 && K.m==1){ P.l-=1;}
 if(K.d==1 && K.m==1){ P.l+=1;}
 if(K.w==1 && K.m==1){ P.z-=4;}
 if(K.s==1 && K.m==1){ P.z+=4;}
}

void clearBackground() 
{int x,y;
 for(y=0;y<SH;y++)
 { 
  for(x=0;x<SW;x++){ pixel(x,y,8);} //clear background color
 }	
}

void clipBehindPlayer(int *x1,int *y1,int *z1, int x2,int y2,int z2) //clip line
{
 float da=*y1;                                 //distance plane -> point a
 float db= y2;                                 //distance plane -> point b
 float d=da-db; if(d==0){ d=1;}
 float s = da/(da-db);                         //intersection factor (between 0 and 1)
 *x1 = *x1 + s*(x2-(*x1));
 *y1 = *y1 + s*(y2-(*y1)); if(*y1==0){ *y1=1;} //prevent divide by zero 
 *z1 = *z1 + s*(z2-(*z1));
}

void drawWall(int x1,int x2, int b1,int b2, int t1,int t2, int c, int s)
{int x,y;
 //Hold difference in distance
 int dyb  = b2-b1;                       //y distance of bottom line
 int dyt  = t2-t1;                       //y distance of top    line
 int dx   = x2-x1; if( dx==0){ dx=1;}    //x distance
 int xs=x1;                              //hold initial x1 starting position 
 //CLIP X
 if(x1<   1){ x1=   1;} //clip left
 if(x2<   1){ x2=   1;} //clip left
 if(x1>SW-1){ x1=SW-1;} //clip right
 if(x2>SW-1){ x2=SW-1;} //clip right
 //draw x verticle lines
 for(x=x1;x<x2;x++)
 {
  //The Y start and end point
  int y1 = dyb*(x-xs+0.5)/dx+b1; //y bottom point
  int y2 = dyt*(x-xs+0.5)/dx+t1; //y bottom point
  //Clip Y
  if(y1<   1){ y1=   1;} //clip y 
  if(y2<   1){ y2=   1;} //clip y 
  if(y1>SH-1){ y1=SH-1;} //clip y 
  if(y2>SH-1){ y2=SH-1;} //clip y 
  //surface
  if(S[s].surface== 1){ S[s].surf[x]=y1; continue;} //save bottom points
  if(S[s].surface== 2){ S[s].surf[x]=y2; continue;} //save top    points
  if(S[s].surface==-1){ for(y=S[s].surf[x];y<y1;y++){ pixel(x,y,S[s].c1);};} //bottom
  if(S[s].surface==-2){ for(y=y2;y<S[s].surf[x];y++){ pixel(x,y,S[s].c2);};} //top
  for(y=y1;y<y2;y++){ pixel(x,y,c);} //normal wall
 }
}

int dist(int x1,int y1, int x2,int y2)
{
 int distance = sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
 return distance;
}

void draw3D()
{int s,w,loop, wx[4],wy[4],wz[4]; float CS=M.cos[P.a], SN=M.sin[P.a]; 
//order sectors by distance
 for(s=0;s<numSect-1;s++)                                          
 {    
  for(w=0;w<numSect-s-1;w++)
  {
   if(S[w].d<S[w+1].d)
   { 
    sectors st=S[w]; S[w]=S[w+1]; S[w+1]=st; 
   }
  }
 }

 //draw sectors
 for(s=0;s<numSect;s++)
 { 
  S[s].d=0; //clear distance
       if(P.z<S[s].z1){ S[s].surface=1;}  //bottom surface
  else if(P.z>S[s].z2){ S[s].surface=2;}  //top    surface
  else                { S[s].surface=0;}  //no     surfaces
  for(loop=0;loop<2;loop++)
  {
   for(w=S[s].ws;w<S[s].we;w++)
   {
    //offset bottom 2 points by player
    int x1=W[w].x1-P.x, y1=W[w].y1-P.y;
    int x2=W[w].x2-P.x, y2=W[w].y2-P.y;
    //swap for surface
    if(loop==0){ int swp=x1; x1=x2; x2=swp; swp=y1; y1=y2; y2=swp;}
    //world X position 
    wx[0]=x1*CS-y1*SN;                                                  
    wx[1]=x2*CS-y2*SN; 
    wx[2]=wx[0];                          //top line has the same x
    wx[3]=wx[1]; 
    //world Y position (depth)
    wy[0]=y1*CS+x1*SN; 
    wy[1]=y2*CS+x2*SN;
    wy[2]=wy[0];                          //top line has the same y 
    wy[3]=wy[1]; 
    S[s].d+=dist(0,0, (wx[0]+wx[1])/2, (wy[0]+wy[1])/2 );  //store this wall distance
    //world z height
    wz[0]=S[s].z1-P.z+((P.l*wy[0])/32.0);
    wz[1]=S[s].z1-P.z+((P.l*wy[1])/32.0);
    wz[2]=wz[0]+S[s].z2;                       //top line has new z 
    wz[3]=wz[1]+S[s].z2; 
    //dont draw if behind player
    if(wy[0]<1 && wy[1]<1){ continue;}      //wall behind player, dont draw
    //point 1 behind player, clip
    if(wy[0]<1)
    { 
     clipBehindPlayer(&wx[0],&wy[0],&wz[0], wx[1],wy[1],wz[1]); //bottom line
     clipBehindPlayer(&wx[2],&wy[2],&wz[2], wx[3],wy[3],wz[3]); //top line
    }
    //point 2 behind player, clip
    if(wy[1]<1)
    { 
     clipBehindPlayer(&wx[1],&wy[1],&wz[1], wx[0],wy[0],wz[0]); //bottom line
     clipBehindPlayer(&wx[3],&wy[3],&wz[3], wx[2],wy[2],wz[2]); //top line
    }
    //screen x, screen y position
    wx[0]=wx[0]*200/wy[0]+SW2; wy[0]=wz[0]*200/wy[0]+SH2;  
    wx[1]=wx[1]*200/wy[1]+SW2; wy[1]=wz[1]*200/wy[1]+SH2;
    wx[2]=wx[2]*200/wy[2]+SW2; wy[2]=wz[2]*200/wy[2]+SH2;  
    wx[3]=wx[3]*200/wy[3]+SW2; wy[3]=wz[3]*200/wy[3]+SH2;
    //draw points
    drawWall(wx[0],wx[1], wy[0],wy[1], wy[2],wy[3], W[w].c, s);
   }
   S[s].d/=(S[s].we-S[s].ws); //find average sector distance
   S[s].surface*=-1;          //flip to negative to draw surface
  }
 }
}

void display() 
{int x,y;
 if(T.fr1-T.fr2>=50)                        //only draw 20 frames/second
 { 
  clearBackground();
  movePlayer();
  draw3D(); 

  T.fr2=T.fr1;   
  glutSwapBuffers(); 
  glutReshapeWindow(GLSW,GLSH);             //prevent window scaling
 }

 T.fr1=glutGet(GLUT_ELAPSED_TIME);          //1000 Milliseconds per second
 glutPostRedisplay();
} 

void KeysDown(unsigned char key,int x,int y)   
{ 
 if(key=='w'==1){ K.w =1;} 
 if(key=='s'==1){ K.s =1;} 
 if(key=='a'==1){ K.a =1;} 
 if(key=='d'==1){ K.d =1;} 
 if(key=='m'==1){ K.m =1;} 
 if(key==','==1){ K.sr=1;} 
 if(key=='.'==1){ K.sl=1;} 
}
void KeysUp(unsigned char key,int x,int y)
{ 
 if(key=='w'==1){ K.w =0;}
 if(key=='s'==1){ K.s =0;}
 if(key=='a'==1){ K.a =0;}
 if(key=='d'==1){ K.d =0;}
 if(key=='m'==1){ K.m =0;}
 if(key==','==1){ K.sr=0;} 
 if(key=='.'==1){ K.sl=0;}
}

int loadSectors[]=
{//wall start, wall end, z1 height, z2 height, bottom color, top color
 0,  4, 0, 40, 2,3, //sector 1
 4,  8, 0, 40, 4,5, //sector 2
 8, 12, 0, 40, 6,7, //sector 3
 12,16, 0, 40, 0,1, //sector 4
};

int loadWalls[]=
{//x1,y1, x2,y2, color
  0, 0, 32, 0, 0,
 32, 0, 32,32, 1,
 32,32,  0,32, 0,
  0,32,  0, 0, 1,

 64, 0, 96, 0, 2,
 96, 0, 96,32, 3,
 96,32, 64,32, 2,
 64,32, 64, 0, 3,

 64, 64, 96, 64, 4,
 96, 64, 96, 96, 5,
 96, 96, 64, 96, 4,
 64, 96, 64, 64, 5,

  0, 64, 32, 64, 6,
 32, 64, 32, 96, 7,
 32, 96,  0, 96, 6,
  0, 96,  0, 64, 7,
};

void init()
{int x;
 //store sin/cos in degrees
 for(x=0;x<360;x++)                         //precalulate sin cos in degrees
 {
  M.cos[x]=cos(x/180.0*M_PI); 
  M.sin[x]=sin(x/180.0*M_PI);
 } 
 //init player
 P.x=70; P.y=-110; P.z=20; P.a=0; P.l=0;    //init player variables
 //load sectors
 int s,w,v1=0,v2=0;
 for(s=0;s<numSect;s++)
 {
  S[s].ws=loadSectors[v1+0];                   //wall start number
  S[s].we=loadSectors[v1+1];                   //wall end   number
  S[s].z1=loadSectors[v1+2];                   //sector bottom height
  S[s].z2=loadSectors[v1+3]-loadSectors[v1+2]; //sector top    height
  S[s].c1=loadSectors[v1+4];                   //sector top    color
  S[s].c2=loadSectors[v1+5];                   //sector bottom color
  v1+=6;
  for(w=S[s].ws;w<S[s].we;w++)
  {
   W[w].x1=loadWalls[v2+0]; //bottom x1
   W[w].y1=loadWalls[v2+1]; //bottom y1
   W[w].x2=loadWalls[v2+2]; //top    x2
   W[w].y2=loadWalls[v2+3]; //top    y2
   W[w].c =loadWalls[v2+4]; //wall color
   v2+=5;
  }
 }

}

int main(int argc, char* argv[])
{
 glutInit(&argc, argv);
 glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
 glutInitWindowPosition(GLSW/2,GLSH/2);
 glutInitWindowSize(GLSW,GLSH);
 glutCreateWindow(""); 
 glPointSize(pixelScale);                        //pixel size
 gluOrtho2D(0,GLSW,0,GLSH);                      //origin bottom left
 init();
 glutDisplayFunc(display);
 glutKeyboardFunc(KeysDown);
 glutKeyboardUpFunc(KeysUp);
 glutMainLoop();
 return 0;
} 

