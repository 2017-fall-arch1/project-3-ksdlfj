/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */
#include <string.h>
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#define GREEN_LED BIT6
int s = 0;
int gameOver = 0;
AbRect rect10 = {abRectGetBounds, abRectCheck, {3,20}}; /**< 3x20 rectangle */
AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};
AbRect rect11 = {abRectGetBounds, abRectCheck, {1, 70}};
Layer wall = {
  (AbShape *) &rect11,
  {116, (screenHeight/2)},
{0,0},{0,0},
  COLOR_WHITE,
  0
};
Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  &wall
};

Layer layer1 = {		/**< Layer with a red rectangle */
  (AbShape *)&rect10,
  {15, screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &fieldLayer,
};

Layer layer0 = {       	/**< Layer with a white circle*/
  (AbShape *)&circle3,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &layer1,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml1 = { &layer1, {0,0}, 0 }; 
MovLayer ml0 = { &layer0, {3,3}, &ml1 }; 

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) || abShapeCheck(&rect10,&layer1.pos,&layer0.pos) == 1) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	if(shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[axis]){
	  gameOver++;
	}
	if(shapeBoundary.botRight.axes[axis] > fence->botRight.axes[0] && !(shapeBoundary.botRight.axes[axis] > fence->botRight.axes[1])){
	  s++;
	  int count = 0;
	  while(++count != 250){
	    P1OUT |= GREEN_LED;
	  }
	  P1OUT &= ~GREEN_LED;
	}
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}


u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */

void movePaddle(u_int switches){
  Vec2 lastPos = layer1.posNext;
  Vec2 v = {15,11};
  Vec2 v2 = {15, 149};
  if(!(switches & (1<<3))){
    if(abRectCheck(&rect10, &lastPos,&v2) == 0){
      changeState(lastPos,0);
    }
   }
  if(!(switches & (1<<0))){
    if(abRectCheck(&rect10, &lastPos, &v) == 0){
      changeState(lastPos,1);
    }
  }
}

void changeState(Vec2 lastPos, int state){
  switch(state){
  case(0):
    ml1.layer->posNext.axes[1] += 5;
    ml1.layer->posLast =lastPos;
    break;
  case(1):
     ml1.layer->posNext.axes[1] -= 5;
     ml1.layer->posLast = lastPos;
     break;
  default: break;
  }
}
/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */

void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */
  P1OUT &= ~GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);

  layerGetBounds(&fieldLayer, &fieldFence);

  char *score = "Score: ";
  char str[15];
  for(int i = 0; i < 15; i++){
    str[i] = " ";
  }
  strcpy(str,score);
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);
  /**< GIE (enable interrupts) */
  
  
  for(;;) {
    str[7] = '0'+s;
    drawString5x7(12,1,str, COLOR_WHITE, COLOR_BLACK);
    u_int switches = p2sw_read();
    
    movePaddle(switches);
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      or_sr(0x10);          /**< CPU OFF */
    }
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
    if(gameOver == 1){
      drawString5x7((screenWidth/2)-20,(screenHeight/2)-20,"YOU LOSE!",COLOR_RED,COLOR_BLACK);
      break;
    }
    if(s == 10){
      drawString5x7((screenWidth/2)-20,(screenHeight/2)-20,"YOU WIN!",COLOR_GREEN,COLOR_BLACK);
      break;
    }
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  
  count ++;
  if (count == 15) {
    mlAdvance(&ml0, &fieldFence);
    if (p2sw_read())
      redrawScreen = 1;
    count = 0;
  } 
}
