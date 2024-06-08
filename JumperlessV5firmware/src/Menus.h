#ifndef MENUS_H
#define MENUS_H



void readMenuFile(void);
void parseMenuFile(void);



void initMenu(void);    
int clickMenu(int menuType = -1 , int menuOption = -1, int extraOptions = 0);
int getMenuSelection(void);
int selectSubmenuOption(int menuPosition, int menuLevel);
int selectNodeAction(int whichSelection = 0);

char printMainMenu(int extraOptions = 0);
char LEDbrightnessMenu();


int findSubMenu(int level, int index);








#endif