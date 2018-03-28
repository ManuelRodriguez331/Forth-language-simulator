/*
titel: "Forth language simulator"
description: Two stack pushdown automaton animated with SFML. The Forth language
  is executed in singlestep mode. Only a few commands of Forth are 
  implemented.
date: 28 March 2018

tutorial:
  1. compile: g++ -std=c++14 -lsfml-graphics -lsfml-window -lsfml-system -pthread fls.cpp
  2. ./a.out
  3. [up] [down] = move instruction pointer
  4. [left] = switch to next cpu
  5. [right] execute command
  6. "run" activates macro for running both cpu

changelog:
- 
to do: 
  - reprogram with GTK+ (supports textinput)

*/
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <complex>
#include <math.h>
#include <random>
#include <thread> 
#include <fstream> // textfile

class Settings {
  // sf::Color black(0x000000ff); // 0xRRGGBBAA, color hexadecimal
  //std::cout << "hallo" << "\n";
  // std::vector<std::string> event = {""};
  //event.push_back("event1");
public:
  sf::RenderWindow window;
  int fps=30;
  int framestep=0;
  sf::Vector2f mouse = {0,0};
  sf::Font font1, font2;
  sf::Text text;
  std::vector<std::string> guimessage;
  std::string screenout;
  /// Settings constructor
  Settings()
  {
    font1.loadFromFile("/usr/share/fonts/liberation/LiberationSans-Regular.ttf");
//    font.loadFromFile("/usr/share/fonts/urw-base35/NimbusSans-Regular.otf");
    font2.loadFromFile("/usr/share/fonts/liberation/LiberationMono-Regular.ttf");
    
    text.setFont(font2);
    text.setCharacterSize(16);
    text.setFillColor(sf::Color::Black);
  }
  auto random_integer(int min, int max) {
    return min + rand() % (( max + 1 ) - min); // fast random generator
  }
  /// paint text s on x,y
  void painttext(std::string s, int x, int y) {
    text.setString(s);
    text.setPosition(x,y);
    window.draw(text);
  }
  /// Euclidean ordinary distance
  auto calcdistance(sf::Vector2f p1, sf::Vector2f p2) {
    return std::sqrt(std::pow( p1.x-p2.x, 2.0 ) + std::pow( p1.y-p2.y, 2.0 ) );
  }
  /// polar coordinates for a point on circle
  auto polarpoint(sf::Vector2f p1, double angle, double radius) {
    angle = (angle-90)*M_PI/180;
    sf::Vector2f result;
    result.x = p1.x + radius * cos(angle);
    result.y = p1.y + radius * sin(angle);
    return result;
  }
  /// angle between tww points
  auto angle_between_two_points(sf::Vector2f p1, sf::Vector2f p2) {
    auto angle = atan2(p2.y-p1.y,p2.x-p1.x);
    angle = angle * 180/M_PI; // degree
    angle = angle + 90;
    if (angle<0) angle = angle + 360;
    return angle;
  }
  void drawline(sf::Vector2f p1, sf::Vector2f p2) {
    sf::Vertex line[] = {
      sf::Vertex(p1,sf::Color::Black),
      sf::Vertex(p2,sf::Color::Black)
    };
    window.draw(line, 2, sf::Lines);
  }
  void drawcircle(sf::Vector2f pos, int radius) {
    sf::CircleShape circle(radius);
    //circle.setFillColor(sf::Color::Black);
    circle.setFillColor(sf::Color::Transparent);
    circle.setOutlineColor(sf::Color::Black);
    circle.setPosition(pos);
    circle.setOutlineThickness(1);
    window.draw(circle);
  }
  /// checks if pcheck is in rectangle
  auto inrect(sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f pcheck) {
    auto rangex=false,rangey=false;
    if (pcheck.x>=p1.x and pcheck.x<=p2.x) rangex=true;
    if (pcheck.y>=p1.y and pcheck.y<=p2.y) rangey=true;
    if (rangex==true and rangey==true) return true;
    else return false;
  }
  /// return difference between two angles
  auto anglediff(int source, int target) {
    auto temp = target-source;
    temp = (temp + 180) % 360 - 180;
    return temp;
  }
  auto degreetoradians(float degree) {
    return degree*M_PI/180;
  }
  auto radianstodegree(float radians) {
    return radians*180/M_PI; 
  }
  void savetofile() {
    std::ofstream myfile;
    myfile.open ("rdf.txt");
    myfile << "hello world\n";
    myfile << "subject " << "predicate "<< "object\n";
    myfile.close();
  }

};

Settings mysettings; /// global variables

class CPU {
public:
  int memoryadress=0;
  std::string tapevalue;
  std::vector<std::string> tape;
  std::vector<int> datastack = {};
  std::vector<int> returnstack = {};
  void paint(sf::Vector2f pos, int tapewidth) {
    sf::Vector2f p1;
    int cellheight=20;
    // datastack marker
    p1={pos.x-14,pos.y+memoryadress*cellheight+5};
    mysettings.drawcircle(p1,4); 
    // datastack value
    for (auto i=0;i<datastack.size();i++) {
      p1={pos.x-30-i*15, pos.y+memoryadress*cellheight};
      mysettings.painttext(std::to_string(datastack[i]),p1.x,p1.y);    
    }
    // returnstack marker
    p1={pos.x+tapewidth+5,pos.y+memoryadress*cellheight+5};
    mysettings.drawcircle(p1,4); 
    // returnstack value
    for (auto i=0;i<returnstack.size();i++) {
      p1={pos.x+tapewidth+20+i*15, pos.y+memoryadress*cellheight};
      mysettings.painttext(std::to_string(returnstack[i]),p1.x,p1.y);    
    }
  }
  void fetch(std::string tapevalue_,std::vector<std::string> tape_) {
    tapevalue=tapevalue_;
    tape=tape_;
    std::cout<<"interrupt: "<<"no, ";
    std::cout<<"fetch: ";
    std::cout<<"adress "<<memoryadress<<" ";
    std::cout<<"tapevalue "<<tapevalue<<" ,";
  }
  void decode() {
    std::string result;
    std::cout<<"decode: ";  
    if (std::isdigit(tapevalue.at(0))==true) {// check if first element is a number
      result="dspush"; // is number -> push on stack
      datastack.push_back(std::stoi(tapevalue));
    }
    else if (tapevalue.at(0)==':')
      result="NOP";// is word def -> NOP
    else if (tapevalue.at(0)==';')  {
      result="return"; //  end of word -> return from subfunction
      auto goaladress = returnstack.back(); // last element
      returnstack.pop_back(); // remove last element
      memoryadress=goaladress; // jump to goal
    }
    else if (tapevalue==".")  {
      result="print stack"; // print symbol -> print stack
      if (mysettings.screenout.length()%10==0)
        mysettings.screenout+="\n"; // linebreak      
      mysettings.screenout+=std::to_string(datastack.back()); // print last element
      datastack.pop_back(); // remove last element
    }
    else if (tapevalue=="+")  {
      result="add "; // + symbol -> add
      auto sum = 0;
      sum += datastack.back(); // last element
      datastack.pop_back(); // remove last element
      sum += datastack.back(); // last element
      datastack.pop_back(); // remove last element
      datastack.push_back(sum); // put sum on stack
    }
    else {
      result="gotoword"; // is word -> gotoword
      returnstack.push_back(memoryadress); // currentadress on returnstack
      int goaladress=search(tapevalue); // search for word on tape
      memoryadress=goaladress; // jump to goal
    }
    std::cout<<result<<"\n";    
  }
  int search(std::string name) {
    /// returns index in tape
    int result=-1; // not found
    for (auto i=0;i<tape.size();i++) {
      if (tape[i].at(0)==':') // word definition?
        if (tape[i].at(2)==name.at(0) and tape[i].at(3)==name.at(1)) // two chars equal?
          result=i;
    }
    return result;
  }
};

class Screen {
public:
  sf::Vector2f pos={100,200};
  int width=130, height=150;
  void update() {
    // border
    sf::RectangleShape rectangle(sf::Vector2f(width, height));
    rectangle.setFillColor(sf::Color::Transparent);
    rectangle.setPosition(pos.x,pos.y);
    rectangle.setOutlineColor(sf::Color(19,19,19)); 
    rectangle.setOutlineThickness(1);
    mysettings.window.draw(rectangle); 
    // text
    mysettings.painttext(mysettings.screenout,pos.x,pos.y);    
  }
};

class Tape {
public:
  std::vector<CPU> mycpu = {};
  int cpuselect=0;
  sf::Vector2f pos={400,30};
  int cellheight=20;
  std::vector<std::string> field = {": main", "calc","calc","calc",";",": calc","init","init","init",";",": init","add","add","add",";",": add","1","1","+",".",";"};;
  int width=100;
  int height=cellheight*field.size();
  Tape() {
    mycpu.push_back({});
    mycpu.push_back({});
  }

  void update() {
    // border
    sf::RectangleShape rectangle(sf::Vector2f(width, height));
    rectangle.setFillColor(sf::Color::Transparent);
    rectangle.setPosition(pos.x,pos.y);
    rectangle.setOutlineColor(sf::Color(19,19,19)); 
    rectangle.setOutlineThickness(1);
    mysettings.window.draw(rectangle); 
    // cells
    for (auto i=0;i<field.size();i++) {
      sf::Vector2f p1={pos.x,pos.y+ i*cellheight};
      mysettings.painttext(std::to_string(i),p1.x,p1.y); // memory adress
      int space;
      if (field[i].at(0)==':') space=20; // first char of string
      else space=40;
      mysettings.painttext(field[i],p1.x+space,p1.y);    
    }
    // cpu
    for (auto i=0;i<mycpu.size();i++) {
      mycpu[i].paint(pos,width);
    }
  }
};

class Physics {
public:
  Tape mytape;
  Screen myscreen;
  void update() {
    mytape.update();
    myscreen.update();
  }
  void init() {
  }
  void action(std::string name) {
    if (name=="nextcpu") {
      if (mytape.cpuselect==0) mytape.cpuselect=1;
      else mytape.cpuselect=0;
    }    
    if (name=="ipforward") { 
      mytape.mycpu[mytape.cpuselect].memoryadress+=1;
    }
    if (name=="ipback") { 
      mytape.mycpu[mytape.cpuselect].memoryadress-=1;
    }
    if (name=="fetch") {
      auto adress=mytape.mycpu[mytape.cpuselect].memoryadress;
      auto tapevalue=mytape.field[adress];
      mytape.mycpu[mytape.cpuselect].fetch(tapevalue,mytape.field);
      mytape.mycpu[mytape.cpuselect].decode();
      action("ipforward");
    }    

  }
};

class Environment {
public:
  Physics myphysics;
  void update() {
    myphysics.update();
  }
  void taskrun() {
    int pause=200;
    // cpu 1
    myphysics.action("fetch");
    sf::sleep(sf::milliseconds(pause)); 
    myphysics.action("fetch");
    sf::sleep(sf::milliseconds(pause)); 
    
    for (auto i=0;i<1000;i++) {
      myphysics.action("nextcpu");
      // testing for end
      if (myphysics.mytape.mycpu[myphysics.mytape.cpuselect].memoryadress!=4)
        myphysics.action("fetch");
      sf::sleep(sf::milliseconds(pause)); // wait
    }
  }
  void action(std::string name) {
    if (name=="run") {
      std::thread t1;
      t1 = std::thread(&Environment::taskrun, this);
      t1.detach();
    }
    else myphysics.action(name);
  }
  Environment() {
    myphysics.init();
  }
};

Environment myenv; /// global class

class Inputhandling {
public:
  sf::Event event;
  std::string input;
  void parse() {
    /// reacts to input
    if (input=="r") myenv.action("reset");
    if (input=="run") myenv.action("run");
    input.clear();
  }
  void parsetextinput() {
    /// write keystroke to input variable
    if (event.text.unicode == '\b' && input.size()!=0 ) {// backspace?
      input.pop_back();
    }
    else if (event.text.unicode == '\r') // enter?
      parse();
    else input.push_back((char)event.text.unicode); // add character
  }  
  void update() {
    while(mysettings.window.pollEvent(event)) {
      if (event.type == sf::Event::Closed)
        mysettings.window.close();
      if (event.type == sf::Event::MouseMoved) {
        mysettings.mouse.x = sf::Mouse::getPosition(mysettings.window).x;
        mysettings.mouse.y = sf::Mouse::getPosition(mysettings.window).y;
      }
      if (event.key.code == sf::Keyboard::Num1 and event.type == sf::Event::KeyPressed) {
        ;
      }
      if (event.key.code == sf::Keyboard::Up and event.type == sf::Event::KeyPressed)
        myenv.action("ipback");
      if (event.key.code == sf::Keyboard::Down and event.type == sf::Event::KeyPressed)
        myenv.action("ipforward");
      if (event.key.code == sf::Keyboard::Left and event.type == sf::Event::KeyPressed)
        myenv.action("nextcpu");
      if (event.key.code == sf::Keyboard::Right and event.type == sf::Event::KeyPressed)
        myenv.action("fetch");
        

      // https://stackoverflow.com/questions/25320803/sfml-2-real-time-text-inputno-event-loop
      if (event.type == sf::Event::TextEntered) {
        parsetextinput();
      }
    }
  }  
};


class GUI {
public:
  Inputhandling myinput;
   
  void run() {
    mysettings.window.create(sf::VideoMode(800, 500), "Forth language simulator");
    while(mysettings.window.isOpen())
    {
      mysettings.window.display();
      paintscreen();
      myenv.update();
      myinput.update();
      mysettings.framestep++;
      sf::sleep(sf::milliseconds(1000/mysettings.fps)); // wait
    }
  }
  void paintscreen() {
    mysettings.window.clear(sf::Color::White); // clear
    mysettings.guimessage.clear();
    mysettings.guimessage.push_back("frame "+std::to_string(mysettings.framestep));
    mysettings.guimessage.push_back("mouse "+std::to_string(mysettings.mouse.x)+" "+std::to_string(mysettings.mouse.y));
    mysettings.guimessage.push_back("fps "+std::to_string(mysettings.fps));    
    mysettings.guimessage.push_back("input "+myinput.input); 
   
    // paintguimessage
    const auto starty=10;
    for (auto i=0;i<mysettings.guimessage.size();i++)
      mysettings.painttext(mysettings.guimessage[i],10,starty+i*20);    
  }
};


int main()
{ 
  GUI mygui;
  mygui.run();
  return 0;
}

