#define _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING  1
#include "ScriptManager.h"
#include "dukglue/dukglue.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include <unordered_map> 
#include <queue>
using namespace std;
#include <zmq.hpp>
#ifndef _WIN32
#else
#include <thread>
#include <cassert>
#include <string>
#define sleep(n)    Sleep(n)
#endif
using namespace std::tr1;
#pragma warning(disable : 4996)

/**
 * * * * * * * * * * * * * * *
 *                           *
 *  TIME LINE CLASSES START  *
 *                           *
 * * * * * * * * * * * * * * *
 */

 /**
  * The super class for time line related classes.
  * Refered to this tutorial to learn about virtual functions:
  * https://www.geeksforgeeks.org/virtual-function-cpp/
  */
class Timeline
{
public:
    virtual int getTime() = 0;
    virtual void changeTic(float tic_size) = 0;
    virtual void pause() = 0;
protected:
    float tic_size;
};

/**
* A real time class. Subclass of timeline.
* Referred to this link to discover chrono package types and functions.
* https://en.cppreference.com/w/cpp/chrono
* Furthermore looked at this duration_cast documentation and example to
* learn about finding the difference between two times:
* https://en.cppreference.com/w/cpp/chrono/duration/duration_cast
*/
class RealTime : public Timeline
{
public:
    RealTime(float tic_size)
    {
        this->tic_size = tic_size;
        start_time = std::chrono::system_clock::now();
    }

    int getTime() {
        auto current_time = std::chrono::system_clock::now();
        std::chrono::duration<float, std::milli> time_diff = current_time - start_time;
        float tics = (time_diff.count()) / (tic_size * 50);
        return floor(tics);
    }

    void pause() {
        pause_time = std::chrono::system_clock::now();
    }

    int getPausedTime() {
        auto current_time = std::chrono::system_clock::now();
        std::chrono::duration<float, std::milli> time_diff = current_time - pause_time;
        float tics = (time_diff.count()) / (tic_size * 50);
        return floor(tics);
    }

    void changeTic(float tic_size) {
        this->tic_size = tic_size;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::chrono::time_point<std::chrono::system_clock> pause_time;
};

/**
* A real time class. Subclass of timeline.
* Referred to this link to discover chrono package types and functions.
* https://en.cppreference.com/w/cpp/chrono
*/
class GameTime : public Timeline
{
public:
    GameTime(float tic_size, RealTime* realTime)
    {
        this->tic_size = tic_size;
        t = realTime;
        paused_tics = 0;
    }

    int getTime() {
        return (*t).getTime() - paused_tics;
    }

    void pause() {
        if (paused) {
            paused_tics += (*t).getPausedTime();
            paused = false;
        }
        else {
            (*t).pause();
            paused = true;
        }
    }

    bool onPause() {
        return paused;
    }

    void changeTic(float tic_size) {
        float ratio = (this->tic_size) / tic_size;
        this->tic_size = tic_size;
        (*t).changeTic(tic_size);
        paused_tics *= ratio;
    }

    float getTic() {
        return this->tic_size;
    }

private:
    RealTime* t;
    int paused_tics;
    bool paused;
};

/**
 * * * * * * * * * * * * * *
 *                         *
 *  TIME LINE CLASSES END  *
 *                         *
 * * * * * * * * * * * * * *
 */


 /**
  * * * * * * * * * * * * *
  *                       *
  *  GAME ENGINE START    *
  *                       *
  * * * * * * * * * * * * *
  */

  // Engine Enums
enum bodyType { SINGLE_SHAPE, MULTI_SHAPE };
enum materials { LETHAL, SOLID, BOUND, AVATAR_LETHAL, AVATAR_SOLID, AVATAR_OTHER, OTHER };
enum actions { STILL, FALL, JUMP, LEFT, RIGHT, UP, DOWN, PUSH, DIE, SCROLL_RIGHT, SCROLL_LEFT };
enum jumpStatus { NONE, FALLING, JUMPING };
enum slideStatus { SLIDING_RIGHT, SLIDING_LEFT, SLIDING_UP, SLIDING_DOWN };

// Engine Classes
class GameObject; // Game Objects
class Renderer; // Render Object / Objects To Window
class CollisionDetector; // Detects Collisions & Collision Types
class SkinProvider; // Provides Skin To The Object

/** The Game Object Superclass*/
class GameObject
{
public:

    /** Returns the body of the shape */
    virtual sf::Shape* getBody() {
        sf::CircleShape point(1);
        return &point;
    }

    /** Returns the shapes that make up the body of the game object */
    virtual std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(3);
        sf::CircleShape point(1);
        bodies.push_back(&point);
        return bodies;
    }

    /** Returns the material of the shape */
    int getMaterial() {
        return material;
    }

    /** Get the X coordinate in respect to the game's world */
    int getCoordX() {
        return coordX;
    }

    /** Get the Y coordinate in respect to the game's world */
    int getCoordY() {
        return coordY;
    }

    /** Returns the X coordinate (or coordinates) as an array */
    virtual std::vector<int> getCoordinatesX() {
        std::vector<int> xcoords(1);
        xcoords.push_back(coordX);
        return xcoords;
    }

    /** Returns the Y coordinate (or coordinates) as an array  */
    virtual std::vector<int> getCoordinatesY() {
        std::vector<int> ycoords(1);
        ycoords.push_back(coordY);
        return ycoords;
    }

    virtual void render(sf::RenderWindow& window, int transform) {
        // Nothing Yet
    }

    /** Get the X & Y coordinates in the world */
    void setCoordinates(int newX, int newY) {
        this->coordX = newX;
        this->coordY = newY;
    }

    /** Returns The Object's Action*/
    virtual int getAction() {
        return this->action;
    }

    virtual std::string toString() {
        return std::to_string(id);
    }

protected:

    int id;
    int material;
    int bodyType;
    int coordX;
    int coordY;
    int action;
    Renderer* renderer;
    CollisionDetector* collisionDetector;
    SkinProvider* skinProvider;
    //MovementManager* movementManager;
};

/** A Rendering Tool */
class Renderer
{
public:
    // Draws A Single Object To The Window 
    void render(sf::RenderWindow& window, sf::Shape* body, int x, int y, int transform) {
        (*body).setPosition(x + transform, y);
        window.draw(*body);
    }

    // Draws A Collection Of Objects To The Window
    void renderCollection(sf::RenderWindow& window, GameObject* object, int transform) {

        std::vector<sf::Shape*> bodies = (*object).getBodyCollection();
        std::vector<int> xcoords = (*object).getCoordinatesX();
        std::vector<int> ycoords = (*object).getCoordinatesY();

        std::vector<sf::Shape*>::iterator itShape = bodies.begin();
        std::vector<int>::iterator itX = xcoords.begin();
        std::vector<int>::iterator itY = ycoords.begin();

        while (itShape != bodies.end() && itX != xcoords.end() && itY != ycoords.end()) {
            if (*itShape == NULL || *itX == NULL || *itY == NULL) {
                if (*itShape == NULL) {
                    ++itShape;
                }
                if (*itX == NULL) {
                    ++itX;
                }
                if (*itY == NULL) {
                    ++itY;
                }
            }
            else {
                //std::cout << "got here" << endl;
                sf::Shape* body = *itShape;
                int x = *itX;
                int y = *itY;
                render(window, body, x, y, transform);
                ++itShape;
                ++itX;
                ++itY;
            }

        }
    }
};

/** A Tool For Detecting Collisions */
class CollisionDetector
{
public:
    // Detects Collisons & Returns The Collision Response
    std::vector<int> collide(std::vector<GameObject*> otherObjects, GameObject* thisObject, int action) {

        //std::cout << "Action " << action << std::endl;
        if (action == STILL || action == FALL) // check below for falling 
        {
            (*(*thisObject).getBody()).move(0, 5);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(0, -5);
            std::vector<int> ret = intersection(otherObjects, myBounds, action);
            return ret;
        }

        if (action == JUMP) // check above for jump / Up collision
        {
            (*(*thisObject).getBody()).move(0, -20);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(0, 20);
            std::vector<int> ret = intersection(otherObjects, myBounds, action);
            return ret;
        }

        if (action == UP) // check above for jump / Up collision
        {
            (*(*thisObject).getBody()).move(0, -1);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(0, 1);
            std::vector<int> ret = intersection(otherObjects, myBounds, action);
            return ret;
        }

        if (action == RIGHT) // check the right for right collision
        {
            (*(*thisObject).getBody()).move(8, -5);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(-8, 5);
            std::vector<int> ret = intersection(otherObjects, myBounds, action);
            //std::cout << "Right Requested; Result Is- Still(0) Fall(1) RIGHT(4) LEFT(3) - " << ret << std::endl;
            return ret;
        }

        if (action == LEFT) // check left for left collision
        {
            (*(*thisObject).getBody()).move(-8, -5);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(8, 5);
            std::vector<int> ret = intersection(otherObjects, myBounds, action);
            //std::cout << "Right Requested; Result Is- Still(0) Fall(1) RIGHT(4) LEFT(3) - " << ret << std::endl;
            return ret;
        }
        // std::cout << "EMERGENCY" << std::endl;
        std::vector<int> ret;
        ret.push_back(0);
        ret.push_back(0);
        return ret;
    }

private:

    // Checks Fpr Intersections Between Objects
    std::vector<int> intersection(std::vector<GameObject*> otherObjects, sf::FloatRect myBounds, int action) {
        bool scrollRight = false;
        bool scrollLeft = false;
        std::vector<int> ret;
        ret.push_back(0);
        ret.push_back(0);
        for (std::vector<GameObject*>::iterator it = otherObjects.begin(); it != otherObjects.end(); ++it) {
            int mat = (*it)->getMaterial();
            sf::Shape* body = (*it)->getBody();
            sf::FloatRect otherBounds = (*body).getGlobalBounds();
            if (myBounds.intersects(otherBounds) == true) {
                ret.at(0) = 1;
                if (mat == LETHAL) {
                    //std::cout << "LETHAL DETECTED (1)" << std::endl;
                    ret.at(1) = DIE;
                    return ret;
                }
                else if (mat == SOLID) {
                    //if (action == RIGHT || action == LEFT) {
                        //std::cout << "SOLID DETECTED" << std::endl;
                    //}
                    if (action == STILL || action == FALL || action == RIGHT || action == LEFT) {
                        //std::cout << "Hit Solid - Still(0) Fall(1) RIGHT(4) LEFT(3) - " << action << std::endl;
                        ret.at(1) = STILL;
                        return ret;
                    }
                    else  if (action == JUMP) {
                        ret.at(1) == FALL;
                        return ret;
                        std::cout << "Jump & Hit Solid Fall" << action << std::endl;
                    }
                }
                else if (mat == BOUND) {
                    if (action == RIGHT) {
                        //return SCROLL_RIGHT;
                        //std::cout << "BOUND DETECTED" << std::endl;
                        scrollRight = true;
                    }
                    else if (action == LEFT) {
                        //return SCROLL_LEFT;
                        //std::cout << "BOUND DETECTED" << std::endl;
                        scrollLeft = true;
                    }
                }
                else if (mat == AVATAR_SOLID) {
                    //std::cout << "AVATAR SOLID DETECTED" << std::endl;
                    if (action == UP) {
                        ret.at(1) = PUSH;
                        return ret;
                    }
                }
            }
        }
        if (action == STILL) {
            //std::cout << "Falling" << std::endl;
            ret.at(1) = FALL;
            return ret;
            std::cout << "No Intersection Fall" << action << std::endl;
        }
        if (scrollRight) {
            //std::cout << "Right Scroll" << std::endl;
            ret.at(1) = SCROLL_RIGHT;
            return ret;
        }
        if (scrollLeft) {
            //std::cout << "Left Scroll" << std::endl;
            ret.at(1) = SCROLL_LEFT;
            return ret;
        }
        if (action == JUMP && myBounds.top < 15) {
            //std::cout << "Ceiling Intersect" << action << std::endl;
            ret.at(0) = 1;
            ret.at(1) = FALL;
            return ret;
        }
        ret.at(1) = action;
        return ret;
    }
};

/** Horizontal Movement Manager Class */
class HorizontalSliderMovementManager
{
public:
    // The Default Constructor 
    HorizontalSliderMovementManager() {
        this->maxRight = 0;
        this->minLeft = 700;
        this->leftLength = 0;
        this->rightLength = 50;
        this->movingRight = true;
    }

    // The Preferred Constructor 
    HorizontalSliderMovementManager(int left, int right, int rightLength, int leftLength, int action) {
        this->maxRight = right;
        this->minLeft = left;
        this->leftLength = leftLength;
        this->rightLength = rightLength;
        if (action == RIGHT) {
            this->movingRight = true;
        }
        else {
            this->movingRight = false;
        }
    }

    // The Next Position 
    int* nextPosition(int x, int y) {
        int ret[2];
        ret[0] = x;
        ret[1] = y;
        if (movingRight) { // moving right
            if ((x + leftLength + 1) <= maxRight) { // w/in range
                ret[0]++;
            }
            else { // hit boundary
                movingRight = false;
            }
        }
        else { // moving Left
            if ((x - rightLength - 1) >= minLeft) { // w/in range
                ret[0]--;
            }
            else { // hit boundary
                movingRight = true;
            }
        }

        return ret;
    }

    // The Slider Status
    int getStatus() {
        if (movingRight) {
            return SLIDING_RIGHT;
        }
        return SLIDING_LEFT;
    }

private:
    bool movingRight;
    int maxRight;
    int minLeft;
    int leftLength;
    int rightLength;
};

/** A Class / Tool For Managing Verticle Sliding Movement */
class VerticalSliderMovementManager
{
public:
    // The Default Constructor 
    VerticalSliderMovementManager() {
        this->minUp = 0;
        this->maxDown = 450;
        this->lowerLength = 50;
        this->upperLength = 0;
        this->movingUp = true;
    }

    // The Preferred Constructor
    VerticalSliderMovementManager(int up, int down, int upLength, int downLength, int action) {
        this->minUp = up;
        this->maxDown = down;
        this->lowerLength = downLength;
        this->upperLength = upLength;
        if (action == UP) {
            this->movingUp = true;
        }
        else {
            this->movingUp = false;
        }
    }

    // The Next Position
    int* nextPosition(int x, int y) {
        int ret[2];
        ret[0] = x;
        ret[1] = y;
        if (movingUp) { // moving up
            if ((y - upperLength - 1) > minUp) { // w/in range
                ret[1]--;
            }
            else { // hit boundary
                movingUp = false;
            }
        }
        else { // moving down
            if ((y + lowerLength + 1) < maxDown) { // w/in range
                ret[1]++;
            }
            else { // hit boundary
                movingUp = true;
            }
        }
        return ret;
    }

    // The Status Of The Slider
    int getStatus() {
        if (movingUp) {
            return SLIDING_UP;
        }
        return SLIDING_DOWN;
    }

private:
    bool movingUp;
    int minUp;
    int maxDown;
    int lowerLength;
    int upperLength;
};

/** A Class / Tool For Managing Jumping Falling Movement */
class JumpingFallingMovementManager
{
public:
    // The Default Constructor
    JumpingFallingMovementManager() {
        this->jumping = false;
        this->falling = false;
        this->currentJumpSpeed = 2;
        this->startJumpSpeed = 2;
        this->currentFallingSpeed = 2;
        this->startFallingSpeed = 2;
        this->jumpAccel = 0.02;
        this->fallingAccel = 0.001;
    }

    // The Prefered Constructor 
    JumpingFallingMovementManager(float jumpingSpeed, float jumpingAccel, float fallingSpeed, float fallingAccel) {
        this->jumping = false;
        this->falling = false;
        this->currentJumpSpeed = jumpingSpeed;
        this->startJumpSpeed = jumpingSpeed;
        this->currentFallingSpeed = fallingSpeed;
        this->startFallingSpeed = fallingSpeed;
        this->jumpAccel = jumpingAccel;
        this->fallingAccel = fallingAccel;
    }

    // Returns The Next Position Of The Character
    int* nextPosition(int x, int y, int action) {
        int ret[3];
        ret[0] = x;
        ret[1] = y;
        ret[2] = 0;
        if (action == STILL) {
            falling = false;
            jumping = false;
            currentFallingSpeed = startFallingSpeed;
            currentJumpSpeed = startJumpSpeed;
            return ret;
        }
        if (action == JUMP && !falling && y > 0) {
            currentJumpSpeed -= jumpAccel;
            ret[1] -= currentJumpSpeed;
            if (currentJumpSpeed < 0) {
                jumping = true;
                falling = false;
                return ret;
            }
            else {
                ret[1] -= currentJumpSpeed;
                return ret;
            }
        }
        if (action == JUMP && falling || action == JUMP && y <= 0) {
            jumping = false;
            falling = true;
            ret[2] = FALL;
            return ret;
        }
        if (action == FALL) {
            currentFallingSpeed += fallingAccel;
            ret[1] += currentFallingSpeed;
            return ret;
        }
        // std::cout << "action: " << action << std::endl;
        return ret;
    }

    // Returns Whether The Object Is Jumping Or Falling or Neither
    int status() {
        std::cout << "jumping status: " << jumping << std::endl;
        if (jumping) {
            return JUMP;
        }
        else if (falling) {
            return FALL;
        }
        else {
            return STILL;
        }
    }

private:
    bool jumping;
    float currentJumpSpeed;
    float startJumpSpeed;
    float jumpAccel;

    bool falling;
    float currentFallingSpeed;
    float startFallingSpeed;
    float fallingAccel;
};

/** The Super Class For Skin Providers */
class SkinProvider
{
public:
    void virtual wearSkin(sf::Shape* body, std::string stringSkin, int red, int green, int blue) {
        // Nothing Happens
    }

    void virtual wearSkinOutline(sf::Shape* body, std::string stringSkin, int red, int green, int blue) {
        // Nothing Happens
    }
};

/** The Skin Provider For Textured Skin */
class TextureSkinProvider : public SkinProvider
{
public:

    /**
     * https://www.youtube.com/watch?v=NGnjDIOGp8s (loading textures )
     */

     /** Wear A Textured Skin */
    void wearSkin(sf::Shape* body, std::string stringSkin, int red, int green, int blue) {
        sf::Texture texture;
    }

    void wearSkin2(sf::Shape* body, std::string stringSkin, sf::Texture* texture) {

        if (!(*texture).loadFromFile("Textures/dark_wood.JPG"))
        {
            std::cout << "ERROR LOADING IMAGE" << std::endl;
        }
        (*body).setTexture(texture);
    }

    /** The Skin Outline */
    void wearSkinOutline(sf::Shape* body, std::string stringOutline, int red, int green, int blue) {
        (*body).setOutlineThickness(5);
        if (stringOutline.compare("Blue") == 0) {
            (*body).setOutlineColor(sf::Color::Blue);
            return;
        }
        if (stringOutline.compare("Black") == 0) {
            (*body).setFillColor(sf::Color::Black);
            return;
        }
        if (stringOutline.compare("White") == 0) {
            (*body).setOutlineColor(sf::Color::White);
            return;
        }
        if (stringOutline.compare("Red") == 0) {
            (*body).setOutlineColor(sf::Color::Red);
            return;
        }
        if (stringOutline.compare("Green") == 0) {
            (*body).setOutlineColor(sf::Color::Green);
            return;
        }
        if (stringOutline.compare("Yellow") == 0) {
            (*body).setOutlineColor(sf::Color::Yellow);
            return;
        }
        if (stringOutline.compare("Magenta") == 0) {
            (*body).setOutlineColor(sf::Color::Magenta);
            return;
        }
        if (stringOutline.compare("Cyan") == 0) {
            (*body).setOutlineColor(sf::Color::Cyan);
            return;
        }
        if (stringOutline.compare("Transparent") == 0) {
            (*body).setOutlineColor(sf::Color::Transparent);
            return;
        }
        (*body).setOutlineColor(sf::Color::Color(red, green, blue, 255));
    }
};

/** The Skin Provider For Solid Colors */
class ColorSkinProvider : public SkinProvider
{
public:

    /** Add Solid Color Skin */
    void wearSkin(sf::Shape* body, std::string stringSkin, int red, int green, int blue) {
        if (stringSkin.compare("Blue") == 0) {
            (*body).setFillColor(sf::Color::Blue);
            return;
        }
        if (stringSkin.compare("Black") == 0) {
            (*body).setFillColor(sf::Color::Black);
            return;
        }
        if (stringSkin.compare("White") == 0) {
            (*body).setFillColor(sf::Color::White);
            return;
        }
        if (stringSkin.compare("Red") == 0) {
            (*body).setFillColor(sf::Color::Red);
            return;
        }
        if (stringSkin.compare("Green") == 0) {
            (*body).setFillColor(sf::Color::Green);
            return;
        }
        if (stringSkin.compare("Yellow") == 0) {
            (*body).setFillColor(sf::Color::Yellow);
            return;
        }
        if (stringSkin.compare("Magenta") == 0) {
            (*body).setFillColor(sf::Color::Magenta);
            return;
        }
        if (stringSkin.compare("Cyan") == 0) {
            (*body).setFillColor(sf::Color::Cyan);
            return;
        }
        if (stringSkin.compare("Transparent") == 0) {
            (*body).setFillColor(sf::Color::Transparent);
            return;
        }
        (*body).setFillColor(sf::Color::Color(red, green, blue, 255));
    }

    /** Add A Colored Outline */
    void wearSkinOutline(sf::Shape* body, std::string stringOutline, int red, int green, int blue) {
        (*body).setOutlineThickness(5);
        if (stringOutline.compare("Blue") == 0) {
            (*body).setOutlineColor(sf::Color::Blue);
            return;
        }
        if (stringOutline.compare("Black") == 0) {
            (*body).setFillColor(sf::Color::Black);
            return;
        }
        if (stringOutline.compare("White") == 0) {
            (*body).setOutlineColor(sf::Color::White);
            return;
        }
        if (stringOutline.compare("Red") == 0) {
            (*body).setOutlineColor(sf::Color::Red);
            return;
        }
        if (stringOutline.compare("Green") == 0) {
            (*body).setOutlineColor(sf::Color::Green);
            return;
        }
        if (stringOutline.compare("Yellow") == 0) {
            (*body).setOutlineColor(sf::Color::Yellow);
            return;
        }
        if (stringOutline.compare("Magenta") == 0) {
            (*body).setOutlineColor(sf::Color::Magenta);
            return;
        }
        if (stringOutline.compare("Cyan") == 0) {
            (*body).setOutlineColor(sf::Color::Cyan);
            return;
        }
        if (stringOutline.compare("Transparent") == 0) {
            (*body).setOutlineColor(sf::Color::Transparent);
            return;
        }
        (*body).setOutlineColor(sf::Color::Color(red, green, blue, 255));
    }
};

/**
 * * * * * * * * * * * *
 *                     *
 *  GAME ENGINE END    *
 *                     *
 * * * * * * * * * * * *
 */

 /**
  * * * * * * * * * * * * * *
  *                         *
  *  ACTUAL OBJECTS START   *
  *                         *
  * * * * * * * * * * * * * *
  */

  /** A Plank Class */
class Plank : public GameObject
{
public:
    Plank(int id, int x, int y, int length) {

        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = x;
        this->coordY = y;

        // Set Material & Body Type
        this->material = SOLID;
        this->bodyType = SINGLE_SHAPE;

        this->action = STILL;

        // Set Length and Initalize Body
        sf::RectangleShape rectangle(sf::Vector2f(length, 30));
        this->body = rectangle;
        this->length = length;

        // Initialize Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        (*this->skinProvider).wearSkin(&(this->body), "", 121, 81, 38);
        (*this->skinProvider).wearSkinOutline(&(this->body), "", 43, 29, 14);
    }

    // Render to Window
    void render(sf::RenderWindow& window, int transform) {
        (*this->renderer).renderCollection(window, this, transform);
    }

    // Return a Refernece To The Body
    sf::Shape* getBody() {
        return &body;
    }

    // Returns a Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        bodies.push_back(&body);
        return bodies;
    }

private:
    SkinProvider* skinProvider;
    //sf::Texture texture;
    sf::RectangleShape body;
    int length;
};

/** Stone Wall Object */
class StoneWall : public GameObject
{
public:
    StoneWall(int id, int x, int y, int stoneCount) {
        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = x;
        this->coordY = y;

        this->action = STILL;

        // Set Material & Body Type
        this->material = SOLID;
        this->bodyType = MULTI_SHAPE;

        // Initalize Body Collection
        this->stoneCount = stoneCount;
        int stoneCoordY = y;
        int stoneCoordX = x;
        for (int i = 0; i < stoneCount; i++) {
            sf::RectangleShape rectangle(sf::Vector2f(50, 50));
            bodyCollection.push_back(rectangle);
            xCoords.push_back(stoneCoordX);
            yCoords.push_back(stoneCoordY);
            stoneCoordY += 50;
        }

        // Initialize Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        std::vector<sf::RectangleShape>::iterator itShape = bodyCollection.begin();
        for (int i = 0; i < stoneCount; i++) {
            (*this->skinProvider).wearSkin(&(*itShape), "", 208, 208, 208);
            (*this->skinProvider).wearSkinOutline(&(*itShape), "", 80, 80, 80);
            ++itShape;
        }

        // Set The Body Of The Wall Used For Bounds 
        sf::RectangleShape rectangle(sf::Vector2f(50, 50 * stoneCount));
        rectangle.setPosition(x, y);
        this->body = rectangle;
    }

    // Render to Window
    void render(sf::RenderWindow& window, int transform) {
        this->body.setPosition(coordX + transform, coordY);
        (*this->renderer).renderCollection(window, this, transform);
    }

    // Return a Refernece To An Invisible Body For Bounds
    sf::Shape* getBody() {
        return &body;
    }

    // Returns a Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        std::vector<sf::RectangleShape>::iterator itshape = bodyCollection.begin();
        for (std::vector<sf::RectangleShape>::iterator itshape = bodyCollection.begin(); itshape != bodyCollection.end(); ++itshape) {
            bodies.push_back(&(*itshape));
        }
        return bodies;
    }

    // Returns The X Cordinates Of The Stones In The Stone Wall
    std::vector<int> getCoordinatesX() {
        return xCoords;
    }

    // Returns The Y Cordinates Of The Stones In The Stone Wall
    std::vector<int> getCoordinatesY() {
        return yCoords;
    }

private:
    std::vector<int> xCoords;
    std::vector<int> yCoords;
    sf::RectangleShape body;
    std::vector<sf::RectangleShape> bodyCollection;
    int stoneCount;
};

/** Death Zone Class */
class DeathZone : public GameObject
{
public:
    DeathZone(int id, int x, int y, int width, int height) {
        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = x;
        this->coordY = y;

        this->action = STILL;

        // Set Material & Body Type
        this->material = LETHAL;
        this->bodyType = SINGLE_SHAPE;

        // Set Width & Height and Initalize Body
        this->width = width;
        this->height = height;
        sf::RectangleShape rectangle(sf::Vector2f(width, height));
        this->body = rectangle;

        // Initialize Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        (*this->skinProvider).wearSkin(&(this->body), "Transparent", 0, 0, 0);
    }

    // Render to Window
    void render(sf::RenderWindow& window, int transform) {
        (*this->renderer).renderCollection(window, this, transform);
    }

    // Return a Refernece To The Body
    sf::Shape* getBody() {
        return &body;
    }

    // Returns A Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        bodies.push_back(&body);
        return bodies;
    }

private:
    sf::RectangleShape body;
    int width;
    int height;
};

/** Lava Class */
class Lava : public GameObject
{
public:
    Lava(int id, int x, int y, int width, int height) {

        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = x;
        this->coordY = y;

        this->action = STILL;

        // Set Material & Body Type
        this->material = LETHAL;
        this->bodyType = SINGLE_SHAPE;

        // Set Width & Height and Initalize Body
        this->width = width;
        this->height = height;
        sf::RectangleShape rectangle(sf::Vector2f(width, height));
        this->body = rectangle;

        // Initialize Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        (*this->skinProvider).wearSkin(&(this->body), "Red", 0, 0, 0);
        (*this->skinProvider).wearSkinOutline(&(this->body), "", 101, 33, 33);
    }

    // Render to Window
    void render(sf::RenderWindow& window, int transform) {
        (*this->renderer).renderCollection(window, this, transform);
    }

    // Return a Refernece To The Body
    sf::Shape* getBody() {
        return &body;
    }

    // Returns A Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        bodies.push_back(&body);
        return bodies;
    }

private:
    sf::RectangleShape body;
    int width;
    int height;
};

/** CumulusCloud Moving Object*/
class CumulusCloud : public GameObject
{
public:
    CumulusCloud(int id, int startX, int startY, int distance) {

        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = startX;
        this->coordY = startY;

        // Set Material & Body Type
        this->material = SOLID;
        this->bodyType = SINGLE_SHAPE;

        this->action = RIGHT;

        // Set Width & Height and Initalize Body
        sf::RectangleShape rectangle(sf::Vector2f(150, 75));
        this->body = rectangle;

        // Initialize Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;
        HorizontalSliderMovementManager newMovementManager(startX, startX + distance, 0, 150, RIGHT);
        this->movementManager = newMovementManager;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        (*this->skinProvider).wearSkin(&(this->body), "", 244, 255, 255);
        (*this->skinProvider).wearSkinOutline(&(this->body), "", 72, 99, 160);
    }

    // Render to Window
    void render(sf::RenderWindow& window, int transform) {
        updatePosition();
        (*this->renderer).renderCollection(window, this, transform);
    }

    // Returns The Action Of This Moving Platform
    int getAction() {
        this->action = movementManager.getStatus();
        return this->action;
    }

    // Return a Refernece To The Body
    sf::Shape* getBody() {
        return &body;
    }

    // Returns a Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        bodies.push_back(&body);
        return bodies;
    }

private:

    // Updates The Position Of The Moving Platform
    void updatePosition() {
        int* coords = movementManager.nextPosition(coordX, coordY);
        this->coordX = coords[0];
        this->coordY = coords[1];
    }

    HorizontalSliderMovementManager movementManager;
    sf::RectangleShape body;
    int width;
    int height;
};

/** Stratus Cloud Class */
class StratusCloud : public GameObject
{
public:
    StratusCloud(int id, int startX, int startY) {

        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = startX;
        this->coordY = startY;

        // Set Material & Body Type
        this->material = SOLID;
        this->bodyType = SINGLE_SHAPE;

        this->action = DOWN;

        // Set Width & Height and Initalize Body
        sf::RectangleShape rectangle(sf::Vector2f(150, 40));
        this->body = rectangle;

        // Initialize Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;
        VerticalSliderMovementManager newMovementManager(startY, startY + 200, 0, 40, DOWN);
        this->movementManager = newMovementManager;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        (*this->skinProvider).wearSkin(&(this->body), "", 124, 175, 175);
        (*this->skinProvider).wearSkinOutline(&(this->body), "", 21, 27, 84);
    }

    // Render to Window
    void render(sf::RenderWindow& window, int transform) {
        //updatePosition();
        (*this->renderer).renderCollection(window, this, transform);
    }

    // Return a Refernece To The Body
    sf::Shape* getBody() {
        return &body;
    }

    // Returns a Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        bodies.push_back(&body);
        return bodies;
    }

    // ReturnsThe Action Of The Platform
    int getAction() {
        this->action = movementManager.getStatus();
        return this->action;
    }

    // Returns  Whether This Platform Is Pushing The Provided Items
    int push(std::vector<GameObject*> otherObjects) {
        int action = (*this->collisionDetector).collide(otherObjects, this, JUMP).at(1);
        if (action == PUSH) {
            return PUSH;
        }
        return STILL;
    }

private:

    // Update The Stratus CLoud Position In Association With 
    void updatePosition() {
        int* coords = movementManager.nextPosition(coordX, coordY);
        this->coordX = coords[0];
        this->coordY = coords[1];
    }

    VerticalSliderMovementManager movementManager;
    sf::RectangleShape body;
    int width;
    int height;
};

/** Character Object */
class Character : public GameObject
{
public:
    Character(int id, JumpingFallingMovementManager* jfmm, int* jAction) {

        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = 100;
        this->coordY = 100;

        // Set Material & Body Type
        this->material = AVATAR_SOLID;
        this->bodyType = SINGLE_SHAPE;

        // Set The Body
        sf::CircleShape circle{};
        circle.setRadius(22.5);
        circle.setPosition(100, 100);
        this->body = circle;

        // Set The Action
        this->action = STILL;
        this->jumpAction = jAction;
        //(*this->jumpAction) = STILL;
        //std::cout << "jump action (constructor): " << (*this->jumpAction) << std::endl;

        // Set Up The Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;
        //JumpingFallingMovementManager newMovementManager(5, 0.04, 5, 0.01);
        this->movementManager = jfmm;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        (*this->skinProvider).wearSkin(&(this->body), "", 0, 32, 194);
        (*this->skinProvider).wearSkinOutline(&(this->body), "", 21, 27, 84);

        //this->currentJumpSpeed = 2;
        //this->startJumpSpeed = 2;
        //this->jumpAccel = 0.02;
    }

    // Render to Window
    void render(sf::RenderWindow& window, int transform) {
        (*this->renderer).renderCollection(window, this, transform);
    }

    // Return a Refernece To The Body
    sf::Shape* getBody() {
        return &body;
    }

    // Returns a Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        bodies.push_back(&body);
        return bodies;
    }

    // Change The Character's Action To "Jump" If Valid
    void changeJump(int action) {
        if (this->action != FALL) {
            this->action = action;
            //currentJumpSpeed += jumpAccel;
            (*this->jumpAction) = action;
        }
    }

    // Return true if jumping
    bool getJump() {
        if ((*this->jumpAction) == JUMP) {
            return true;
        }
    }

    int checkTranslate(std::vector<GameObject*> otherObjects, int action) {
        return (*this->collisionDetector).collide(otherObjects, this, action)[1];
    }

    void moveRight() {
        coordX += 6;
    }

    void moveLeft() {
        coordX -= 6;
    }

    // Translate The Avatar Right / Left  If Valid
    int changeTranslate(std::vector<GameObject*> otherObjects, int action) {
        this->action = (*this->collisionDetector).collide(otherObjects, this, action)[1];
        if (this->action == RIGHT || this->action == SCROLL_RIGHT) {
            coordX += 6;
            return this->action;
        }
        else if (this->action == LEFT || this->action == SCROLL_LEFT) {
            coordX -= 6;
            return this->action;
        }
        else {
            return this->action;
        }
    }

    std::vector<int> checkOnCharacterUpdate(std::vector<GameObject*> otherObjects, int action) {
        if ((*this->jumpAction) == JUMP) {
            action = JUMP;
        }
        //std::cout << "jump action (beganning check on character update): " << (*this->jumpAction) << std::endl;
        std::vector<int> ret;
        ret.push_back(0); // collision 
        ret.push_back(0); // death 
        ret.push_back(0); // action
        if (action == DIE) {
            action == STILL;
        }
        std::vector<int> newAction;
        if ((*this->jumpAction) == JUMP) {
            newAction = (*this->collisionDetector).collide(otherObjects, this, (*this->jumpAction));
        }
        else {
            newAction = (*this->collisionDetector).collide(otherObjects, this, action);
        }
        if (newAction.at(0) == 1) {
            ret.at(0) = 1;
            if (newAction.at(1) == DIE) {
                ret.at(1) = 1;
            }
        }
        ret.at(2) = newAction.at(1);
        //std::cout << "jump action (end check on character update): " << (*this->jumpAction) << std::endl;
        return ret;
    }

    void updateCollisionAction(int action) {

        if (action == FALL) {
            this->action = FALL;
            *this->jumpAction = FALL;


        }
        if (action == STILL) {
            this->action = STILL;
            *this->jumpAction = STILL;
        }

        if (action == FALL || action == STILL) {
            int* coords = (*movementManager).nextPosition(coordX, coordY, (*this->jumpAction));

            this->coordX = coords[0];
            this->coordY = coords[1];
        }
    }

    void changeAction(int action) {
        this->action = action;
    }

    void noCollisionFall() {
        this->action = FALL; // update fall
        (*this->jumpAction) = FALL;
        int* coords = (*movementManager).nextPosition(coordX, coordY, (*this->jumpAction));

        this->coordX = coords[0];
        this->coordY = coords[1];
    }

    void contJump() {
        int* coords = (*movementManager).nextPosition(coordX, coordY, (*this->jumpAction));

        this->coordX = coords[0];
        this->coordY = coords[1];
    }

    void die() {
        this->action = DIE;
    }

    // Update Character Position Based On Jumping / Falling Movement Manager 
    int updatePosition(std::vector<GameObject*> otherObjects, int action) {
        std::vector<int> updateResults;

        updateResults = checkOnCharacterUpdate(otherObjects, action);

        if (updateResults.at(2) == DIE) {// if character dies, kill them
            //this->action = updateResults.at(2);
            //die();
            return -1;
        }
        if (updateResults.at(2) == FALL && updateResults.at(0) == 0) { // if no collision falling 
            // noCollisionFall();
            // return this->action;
            return -1;
        }

        else if (updateResults.at(0) == 1) {

            //updateCollisionAction(updateResults.at(2));
            return -1;
        }
        else if (*this->jumpAction == JUMP) {
            //contJump();
            return -1;
        }

    }

    static void renderOtherCharacter(int x, int y, sf::RenderWindow& window, int transform) {

        sf::CircleShape circle{};
        circle.setRadius(22.5);
        circle.setPosition(x + transform, y);

        ColorSkinProvider skinProvider = ColorSkinProvider{};
        (skinProvider).wearSkin(&(circle), "", 0, 32, 194);
        (skinProvider).wearSkinOutline(&(circle), "", 21, 27, 84);

        window.draw(circle);
    }

private:
    sf::CircleShape body;
    JumpingFallingMovementManager* movementManager;
    int* jumpAction;
};

/** A Spawn Point */
class SpawnPoint : public GameObject
{
public:
    SpawnPoint(int id, int x, int y, int translate) {
        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = x;
        this->coordY = y;

        // Set Material & Body Type
        this->material = OTHER;
        this->bodyType = SINGLE_SHAPE;

        // Set The Action
        this->action = STILL;

        // Set The World Transform To Reach The Spawn Point
        this->transform = translate;

        // Set Up The Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;
    }

    int getTransform() {
        return this->transform;
    }

private:
    int transform;
};

/** A Boundary Object */
class Boundary : public GameObject
{
public:

    Boundary(int id, int x, int y, bool right) {
        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = x;
        this->coordY = y;

        // Set Material & Body Type
        this->material = BOUND;
        this->bodyType = SINGLE_SHAPE;

        // Set The Action
        this->action = STILL;

        this->width = 50;
        this->height = 600;
        sf::RectangleShape rectangle(sf::Vector2f(width, height));
        rectangle.setPosition(x, y);
        this->body = rectangle;

        // Set Up The Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        (*this->skinProvider).wearSkin(&(this->body), "Transparent", 0, 0, 0);

        this->rightBoundary = right;
    }

    bool isRightBoundary() {
        return rightBoundary;
    }

    // Return a Refernece To The Body
    sf::Shape* getBody() {
        return &body;
    }

    // Returns A Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        bodies.push_back(&body);
        return bodies;
    }

private:
    int width;
    int height;
    sf::RectangleShape body;
    bool rightBoundary;
};


/** The Game World */
class World : public GameObject
{
public:

    World() {} // Irelevant Default Const. 

    World(int id)
    {
        // Initialize Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;

        // Initialize Characteristics 
        this->id = id;
        this->material = OTHER;
        this->bodyType = SINGLE_SHAPE;
        this->action = STILL;
        this->coordX = 0;
        this->coordY = 0;

        // Set Up World Translate
        this->worldTranslate = 200;

        // Set Up Other Variables
        this->spawnCount = 0;

        //this->avatarJump = STILL;
        this->onStratus = false;
    }

    // Translate World Elements To The Right
    void scrollRight() {
        worldTranslate -= 6;
    }

    // Translate World Elemts To The Left
    void scrollLeft() {
        worldTranslate += 6;
    }

    // Add A Generic Object To Game Object Array
    void addObject(GameObject* gameObject) {
        collideObjects.push_back(gameObject);
        renderObjects.push_back(gameObject);
    }

    // Add Stratus Cloud To Game Object & Stratus Cloud Array
    void addStratusCloud(StratusCloud* sc) {
        collideObjects.push_back(sc);
        renderObjects.push_back(sc);
        sClouds.push_back(sc);
    }

    // Add Stratus Cloud To Game Object & Stratus Cloud Array
    void addCumulusCloud(CumulusCloud* cc) {
        collideObjects.push_back(cc);
        renderObjects.push_back(cc);
        cClouds.push_back(cc);
    }

    // Add A Boundary
    void addBoundary(Boundary* bound) {
        collideObjects.push_back(bound);
    }

    // Add Spawn Point To Spawn Array
    void addSpawnPoint(SpawnPoint* sp) {
        spawns.push_back(sp);
        this->spawnCount++;
    }

    int getAvatarCoordX() {
        return avatar->getCoordX();
    }

    int getAvatarCoordY() {
        return avatar->getCoordY();
    }

    // Check If Character Is Sitting On Top Of An Upward Moving Stratus Cloud
    bool stratusCollision() {
        std::vector<GameObject*> characterContainer;
        characterContainer.push_back(avatar);
        for (std::vector<StratusCloud*>::iterator it = sClouds.begin(); it != sClouds.end(); ++it) {
            int coll = (*this->collisionDetector).collide(characterContainer, (*it), UP)[0];
            if (coll == 1) {
                this->onStratus = true;
            }
            else {
                this->onStratus = false;
            }
            int response = (*this->collisionDetector).collide(characterContainer, (*it), UP)[1];
            if (response == PUSH && this->onStratus) {
                return true;
            }
        }
        return false;
    }

    // Add The Avatar
    void addAvatar(Character* avatar) {
        this->avatar = avatar;
        this->avatarJump == STILL;
        this->avatarTranslate == STILL;
    }

    void addGameTime(GameTime* gameTime) {
        this->gameTime = gameTime;
    }

    void addTic(int* tic) {
        this->worldTic = tic;
    }

    // Get Random Spawns Translate Value and New Character Coords
    std::vector<int> getSpawnStats() {
        std::vector<int> ret;
        ret.push_back(0); // Translate
        ret.push_back(0); // Character X
        ret.push_back(0); // Character Y
        if (spawnCount > 0) {
            int spawnNumber = rand() % spawnCount + 1;
            int counter = 1;

            for (std::vector<SpawnPoint*>::iterator it = this->spawns.begin(); it != this->spawns.end(); ++it) {
                if (counter == spawnNumber) {
                    int avaX = (*it)->getCoordX();
                    int avaY = (*it)->getCoordY();
                    int translate = (*it)->getTransform();
                    ret.at(0) = translate;
                    ret.at(1) = avaX;
                    ret.at(2) = avaY;
                }
                counter++;
            }
        }
        else {
            ret.at(0) = 0;
            ret.at(1) = 100;
            ret.at(2) = 100;
        }
        return ret;
    }

    void setTranslate(int translate) {
        this->worldTranslate = translate;
    }

    bool userJumpValid() {
        if (this->avatarJump == FALL && this->onStratus == false) {
            if (this->onStratus == false) {
                return false;
            }
        }
        else {
            return true;
        }
    }

    int checkRightAvatarTranslate() {
        return  (*this->avatar).checkTranslate(collideObjects, RIGHT);
    }

    int checkLeftAvatarTranslate() {
        return  (*this->avatar).checkTranslate(collideObjects, LEFT);
    }

    // Change The Avatar's Action
    void changeAvatarAction(int action) {
        if (action == JUMP) {
            if (this->avatarJump == FALL && this->onStratus == false) {
                if (this->onStratus == false) {
                    return;
                }
            }
            else {
                (*this->avatar).changeJump(action);
                avatarJump = action;
            }
        }
        if (action == RIGHT) {
            int result = (*this->avatar).changeTranslate(collideObjects, RIGHT);
            if (result == SCROLL_RIGHT) {
                scrollRight();
            }
        }
        if (action == LEFT) {
            int result = (*this->avatar).changeTranslate(collideObjects, LEFT);
            if (result == SCROLL_LEFT) {
                scrollLeft();
            }
        }
    }

    int getTransform() {
        return this->worldTranslate;
    }

    Character* getAvatar() {
        return avatar;
    }

    GameTime* getGameTime() {
        return gameTime;
    }

    int* getTic() {
        return worldTic;
    }

    // Returns The World Object Array
    std::vector<GameObject*> getObjects()
    {
        return collideObjects;
    }

    // Returns The Stratus Cloud Array
    std::vector<StratusCloud*> getStratusClouds()
    {
        return sClouds;
    }

    // Returns The Cumulus Cloud Array
    std::vector<CumulusCloud*> getCumulusClouds()
    {
        return cClouds;
    }

    void updateAvatarJump(int action) {
        this->avatarJump = action;
    }

    std::vector<int> checkCharacterUpdate() {
        return avatar->checkOnCharacterUpdate(renderObjects, avatarJump);
    }

    void stratusReact() {
        int avY = avatar->getCoordY();
        int avX = avatar->getCoordX();
        avY -= 4;
        if ((*avatar).getAction() == JUMP) {
            avY -= 15;
        }
        avatar->setCoordinates(avX, avY);
    }

    // Renders World Objects In The World
    void render(sf::RenderWindow& window, int transform) {
        for (std::vector<GameObject*>::iterator it = renderObjects.begin(); it != renderObjects.end(); ++it) {
            (*it)->render(window, worldTranslate);
        }
        avatar->render(window, worldTranslate);
    }

private:
    int avatarJump;
    bool onStratus;
    int avatarTranslate;
    int worldTranslate;
    std::vector<GameObject*> collideObjects;
    std::vector<GameObject*> renderObjects;
    std::vector<StratusCloud*> sClouds;
    std::vector<CumulusCloud*> cClouds;
    std::vector<SpawnPoint*> spawns;
    int spawnCount;
    Character* avatar;
    GameTime* gameTime;
    int* worldTic;
};

/**
 * * * * * * * * * * * * *
 *                       *
 *  ACTUAL OBJECTS END   *
 *                       *
 * * * * * * * * * * * * *
*/

/**
 * * * * * * * * * * * * *
 *                       *
 *  EVENTS START         *
 *                       *
 * * * * * * * * * * * * *
*/

/** The Types Of Events */
enum eventType
{
    CHARACTER_COLLISION = 1, // When the character collides with anything
    CHARACTER_JUMP = 2, // When character jumps
    CHARACTER_FALL = 3, // When characters falls as a result of collision/floor absence
    CHARACTER_DEATH = 4,// When the character dies 
    CHARACTER_SPAWN = 5, // When the character spawns
    CCLOUD_TRANSLATE = 6, // When the columbus cloud translates
    SCLOUD_TRANSLATE = 7, // When the stratus cloud translates
    USER_INPUT = 8, // When the user clicks something (&  its not related to recording)
    START_REC = 9, // When the user clicks to  start recording
    END_REC = 10, // When the user ends a recording
    CLIENT_CONNECT = 11 // When the user attempts to end a recording
};

enum keyboardInput { ARROW, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, Shift, Space };

/** Variant Struct */
struct variant
{
    enum Type
    {
        TYPE_TNTEGER = 0,
        TYPE_FLOAT = 1,
        TYPE_GAMEOBJECT_PTR = 2,

    };
    Type mType;
    union
    {
        int m_asInteger;
        float m_asFloat;
        GameObject* m_asGameObjectPtr;
    };
};

/**
 * Event Class
 *
 * Referred to:
 * https://thispointer.com/how-to-iterate-over-an-unordered_map-in-c11/
 * -- for iterating over an unorderd map & this link:
 * http://www.cplusplus.com/reference/unordered_map/unordered_map/
 * -- to learn more about unordered maps.
 */
class Event
{
public:

    Event(eventType type, unordered_map<std::string, variant> argum, int tic, int priority) {
        this->type = type;
        this->arguments = argum;
        this->executionTime = tic;
        this->priority = tic * 20 + priority;
    }

    Event(std::string eventString) {

        std::string delimiter = ",";
        size_t pos = eventString.find(delimiter);
        int event_type = std::stoi(eventString.substr(0, eventString.find(delimiter)));
        this->type = (eventType)event_type;

        eventString.erase(0, pos + delimiter.length());
        pos = eventString.find(delimiter);
        int event_time = std::stoi(eventString.substr(0, eventString.find(delimiter)));
        this->executionTime = event_time;

        eventString.erase(0, pos + delimiter.length());
        pos = eventString.find(delimiter);
        int event_priority = std::stoi(eventString.substr(0, eventString.find(delimiter)));
        this->priority = event_priority;

        eventString.erase(0, pos + delimiter.length());
        pos = eventString.find(delimiter);
        int arguments_size = std::stoi(eventString.substr(0, eventString.find(delimiter)));

        std::vector<variant> variants;
        for (int i = 0; i < arguments_size; i++) {

            variants.push_back(struct variant());

            eventString.erase(0, pos + delimiter.length());
            pos = eventString.find(delimiter);
            std::string argument_name = eventString.substr(0, eventString.find(delimiter));

            eventString.erase(0, pos + delimiter.length());
            pos = eventString.find(delimiter);
            int argument_value_type = std::stoi(eventString.substr(0, eventString.find(delimiter)));

            if (argument_value_type == 1) {

                variants[i].mType = variant::TYPE_TNTEGER;

                eventString.erase(0, pos + delimiter.length());
                pos = eventString.find(delimiter);
                int argument_value = std::stoi(eventString.substr(0, eventString.find(delimiter)));

                variants[i].m_asInteger = argument_value;
                arguments[argument_name] = variants[i];

            }
            else if (argument_value_type == 2) {

                variants[i].mType = variant::TYPE_FLOAT;

                eventString.erase(0, pos + delimiter.length());
                pos = eventString.find(delimiter);
                float argument_value = std::stof(eventString.substr(0, eventString.find(delimiter)));

                variants[i].m_asFloat = argument_value;
                arguments[argument_name] = variants[i];

            }
            else {

                eventString.erase(0, pos + delimiter.length());
                pos = eventString.find(delimiter);
                std::string argument_value = eventString.substr(0, eventString.find(delimiter));

                this->obj_values.push_back(argument_value);
                this->obj_keys.push_back(argument_name);

            }

        }

    }

    std::vector < std::string > getObjectKeys() {
        return this->obj_keys;
    }

    std::vector < std::string > getObjectValues() {
        return this->obj_values;
    }

    eventType getType() {
        return this->type;
    }

    int getTime() {
        return this->executionTime;
    }

    unordered_map<std::string, variant> getArguments() {
        return arguments;
    }

    std::string getString() {
        std::string ret = std::to_string(this->type) + "," + std::to_string(executionTime)
            + "," + std::to_string(priority) + "," + std::to_string(arguments.size());
        for (std::pair<std::string, variant> element : arguments)
        {
            std::string varCategory = element.first;
            int varType = element.second.mType;
            ret += "," + varCategory + ",";
            if (varType == element.second.TYPE_TNTEGER) {
                ret += "1," + std::to_string(element.second.m_asInteger);
            }
            else if (varType == element.second.TYPE_FLOAT) {
                ret += "2," + std::to_string(element.second.m_asFloat);
            }
            else if (varType == element.second.TYPE_GAMEOBJECT_PTR) {
                ret += "3," + element.second.m_asGameObjectPtr->toString();
            }
        }
        return ret;
    }

    int priority;

private:
    std::vector<std::string> obj_keys;
    std::vector<std::string> obj_values;
    eventType type; // type of event
    unordered_map<std::string, variant> arguments; // arguments to complete event 
    int executionTime; // point in time event is executed
};

/**
 * Event Handler Abstract Class
 */
class EventHandler
{
public:
    void virtual onEvent(Event e, World* w) = 0;
    int virtual getId() = 0;
};

/**
 * -- Character Event Handler Class --
 *
 * Handles Events In A Way That Is More Applicable
 * To The CHARACTER in the Wolrd Object
 */
class CharacterEventHandler : public EventHandler
{
public:
    CharacterEventHandler(int id) {
        this->id = id;
    }

    int getId() {
        return this->id;
    }

    void onEvent(Event e, World* world) {

        // Character Collision
        if (e.getType() == CHARACTER_COLLISION) {

            // Stratus Cloud Collision
            if (e.getArguments()["action"].m_asInteger == UP) {
                // Do Nothing
            }

            else if (e.getArguments()["action"].m_asInteger == SCROLL_RIGHT) {
                (*world).getAvatar()->changeAction(RIGHT);
                (*world).getAvatar()->moveRight();
            }

            else if (e.getArguments()["action"].m_asInteger == SCROLL_LEFT) {
                (*world).getAvatar()->changeAction(LEFT);
                (*world).getAvatar()->moveLeft();
            }

            // Other Collision ?
            else {

                int action = e.getArguments()["action"].m_asInteger;
                if (e.getArguments()["auto"].m_asInteger == 1) {

                    (*world).getAvatar()->updateCollisionAction(action);
                }
                else {
                    (*world).getAvatar()->changeAction(action);
                }

            }

        }

        // Character Death
        else if (e.getType() == CHARACTER_DEATH) {
            (*world).getAvatar()->die();
        }

        // Character Spawn
        else if (e.getType() == CHARACTER_SPAWN) {
            int coord_x = e.getArguments()["coord_x"].m_asInteger;
            int coord_y = e.getArguments()["coord_y"].m_asInteger;
            (*world).getAvatar()->setCoordinates(coord_x, coord_y);
        }

        // Character  Fall
        else if (e.getType() == CHARACTER_FALL) {
            (*world).getAvatar()->noCollisionFall();
        }

        // Character Jump
        else if (e.getType() == CHARACTER_JUMP) {
            (*world).getAvatar()->contJump();
        }

        // User Input
        else if (e.getType() == USER_INPUT) {

            int valid = e.getArguments()["valid"].m_asInteger;
            int key = e.getArguments()["key"].m_asInteger;

            if (valid == 1) {

                if (key == ARROW) {

                    int type = e.getArguments()["type"].m_asInteger;

                    if (type == JUMP) {
                        (*world).getAvatar()->changeJump(JUMP);
                    }

                    if (type == RIGHT) {
                        (*world).getAvatar()->changeAction(RIGHT);
                        (*world).getAvatar()->moveRight();
                    }

                    if (type == LEFT) {
                        (*world).getAvatar()->changeAction(LEFT);
                        (*world).getAvatar()->moveLeft();
                    }

                }
                else if (key == Q) {

                    // Quicken Speed Of Replay --- Nothing Here

                }
                else if (key == R) {

                    // Record Replay -- Nothing Here

                }
                else if (key == D) {

                    // Finish Replay -- Nothing Here

                }
            }
        }
    }
private:
    int id;
};

// ------------------- Globals (1) ------------------------ //
World world;

/**
 * * * * * * * * * * * * * * * * * * * * * *
 *                                         *
 *  SCRIPTING SPECIFIC FUNCTIONS START 1   *
 *                                         *
 * * * * * * * * * * * * * * * * * * * * * *
*/

void printMessage(string message) {
    std::cout << message << std::endl;
}

void worldStratusReact() {
    world.stratusReact();
}

void worldScrollLeft() {
    world.scrollLeft();
}

void worldScrollRight() {
    world.scrollRight();
}

void worldUpdateAvatarJump(int jump) {
    world.updateAvatarJump(jump);
}

void worldKillAvatar() {
    world.updateAvatarJump(DIE);
}

void worldSetTranslate(std::string trans) {
    world.setTranslate(atoi(trans.c_str()));
}

/**
 * * * * * * * * * * * * * * * * * * * * * *
 *                                         *
 *  SCRIPTING SPECIFIC FUNCTIONS END 1     *
 *                                         *
 * * * * * * * * * * * * * * * * * * * * * *
*/

/**
 * -- World Event Handler Class --
 *
 * Handles Events In A Way That Is More
 * Applicable To The World Object
 */
class WorldEventHandler : public EventHandler
{
public:
    WorldEventHandler(int id) {
        this->id;
    }

    int getId() {
        return this->id;
    }

    void onEvent(Event e, World* world) {

         // Non-User Input Events Are Handled Via Script
         ScriptManager scripter = ScriptManager();
         dukglue_register_function(scripter.getContext(), &printMessage, "printMessage");
         dukglue_register_function(scripter.getContext(), &worldStratusReact, "worldStratusReact");
         dukglue_register_function(scripter.getContext(), &worldScrollLeft, "worldScrollLeft");
         dukglue_register_function(scripter.getContext(), &worldScrollRight, "worldScrollRight");
         dukglue_register_function(scripter.getContext(), &worldUpdateAvatarJump, "worldUpdateAvatarJump");
         dukglue_register_function(scripter.getContext(), &worldKillAvatar, "worldKillAvatar");
         dukglue_register_function(scripter.getContext(), &worldSetTranslate, "worldSetTranslate");
         scripter.loadScript("scriptFile.js");
         std::string paramer =  e.getString();
         const char* c = paramer.c_str();
         scripter.runScript("handleWorldEvent", 0, 1, c);


        // User Input Events Handled Here
        if (e.getType() == USER_INPUT) {

            int valid = e.getArguments()["valid"].m_asInteger;
            int key = e.getArguments()["key"].m_asInteger;
            if (valid == 1) {

                if (key == ARROW) {

                    int type = e.getArguments()["type"].m_asInteger;

                    if (type == JUMP) {
                        (*world).updateAvatarJump(JUMP);
                    }

                    if (type == RIGHT) {
                        // Nothing ...
                    }

                    if (type == LEFT) {
                        // Nothing ...
                    }

                }

            }
        }

    }
private:
    int id;
};

class CompareEvents {
public:
    bool operator() (const Event& e1, const Event& e2)
    {
        return e1.priority < e2.priority;
    }
};

/**
 * Event Manager
 */
class EventManager
{
public:
    EventManager() {

        registration[CHARACTER_COLLISION] = std::vector<EventHandler*>(); // 1
        registration[CHARACTER_JUMP] = std::vector<EventHandler*>(); // 2
        registration[CHARACTER_FALL] = std::vector<EventHandler*>(); // 3
        registration[CHARACTER_DEATH] = std::vector<EventHandler*>(); // 4
        registration[CHARACTER_SPAWN] = std::vector<EventHandler*>(); // 5
        registration[CCLOUD_TRANSLATE] = std::vector<EventHandler*>(); // 6
        registration[SCLOUD_TRANSLATE] = std::vector<EventHandler*>(); // 7
        registration[USER_INPUT] = std::vector<EventHandler*>(); // 8
        registration[START_REC] = std::vector<EventHandler*>(); // 9
        registration[END_REC] = std::vector<EventHandler*>(); // 10
        registration[CLIENT_CONNECT] = std::vector<EventHandler*>(); // 11

    }

    void registerHandler(eventType type, EventHandler* handler) {
        registration[type].push_back(handler);
    }

    /**
     * Referred to these links for iteration tips.
     * https://riptutorial.com/cplusplus/example/1678/iterating-over-std--vector
     * http://www.cplusplus.com/reference/vector/vector/erase/
     */
    void unregisterHandler(EventHandler* handler, eventType type) {

        int remove_index = -1;
        for (std::size_t i = 0; i < registration[type].size(); ++i) {
            int id = registration[type].at(i)->getId();
            if (id == (*handler).getId()) {
                remove_index = 0;
            }
        }
        if (remove_index != -1) {
            registration[type].erase(registration[type].begin() + remove_index - 1);
        }
    }

    void raiseEvent(Event e) {
        //qEvents.push(e);
        pEvents.push(e);
    }

    void handleEvents(World* world) {
        while (!pEvents.empty())
        {
            Event currentEvent = pEvents.top();
            eventType type = currentEvent.getType();
            for (std::size_t i = 0; i < registration[type].size(); ++i) {
                registration[type].at(i)->onEvent(currentEvent, world);
            }
            pEvents.pop();
        }

    }

    void handleEvents(World* world, int TimePriority) {
        bool done = false;
        while (!pEvents.empty() && done == false)
        {
            Event currentEvent = pEvents.top();
            eventType type = currentEvent.getType();
            int time = currentEvent.getTime();
            if (time <= TimePriority) {
                for (std::size_t i = 0; i < registration[type].size(); ++i) {
                    registration[type].at(i)->onEvent(currentEvent, world);
                }
                pEvents.pop();
            }
            else {
                done == true;
            }
        }
    }

private:
    unordered_map<eventType, std::vector<EventHandler*>> registration;
    priority_queue<Event, std::vector<Event>, CompareEvents> pEvents;
};

/**
 * * * * * * * * * * * * *
 *                       *
 *  EVENTS END           *
 *                       *
 * * * * * * * * * * * * *
*/

// ------------------- Globals (2) ------------------------ //
EventManager eventManager;

/**
 * * * * * * * * * * * * * * * * * * * * * *
 *                                         *
 *  SCRIPTING SPECIFIC FUNCTIONS START 2   *
 *                                         *
 * * * * * * * * * * * * * * * * * * * * * *
*/

void createRaiseEvent(int type, int priority, int args, std::vector<std::string> keys, std::vector<int> values) {
    // create variants
    std::vector<struct variant> vars;
    for (int i = 0; i < args; i++) {

        vars.push_back(struct variant());        
        vars[i].mType = variant::TYPE_TNTEGER;
        vars[i].m_asInteger = values[i];
    }

    // create unordered map
    std::unordered_map<std::string, variant> arg_map;
    for (int i = 0; i < args; i++) {
        arg_map[keys[i]] = vars[i];
    }

    Event ret((eventType)type, arg_map, *world.getTic(), priority);
    eventManager.raiseEvent(ret);
}

bool isRightValid() {
    int reaction = world.checkRightAvatarTranslate();
    int valid = false;
    if (reaction == RIGHT) {
        valid = true;
    }
    return valid;
}

int getRightAction() {
    return world.checkRightAvatarTranslate();
}

bool isLeftValid() {
    int reaction = world.checkLeftAvatarTranslate();
    int valid = false;
    if (reaction == LEFT) {
        valid = true;
    }
    return valid;
}

int getLeftAction() {
    return world.checkLeftAvatarTranslate();
}

bool isJumpValid(){
    int valid = false;
    if (world.userJumpValid()) {
        valid = true;
    }
    return valid;
}


/**
 * * * * * * * * * * * * * * * * * * * * * *
 *                                         *
 *  SCRIPTING SPECIFIC FUNCTIONS END 2     *
 *                                         *
 * * * * * * * * * * * * * * * * * * * * * *
*/


int main()
{
    // Window Parameters
    int x_max = 800;
    int y_max = 600;

    // Connect To Socket
    zmq::context_t context(1);
    zmq::socket_t requester(context, ZMQ_REQ);
    requester.connect("tcp://localhost:5555");

    //  -- --  Request Id & Platforms -- -- \\

    std::unordered_map<std::string, variant> new_client_args;
    Event ccEvent(CLIENT_CONNECT, new_client_args, 0, 0);

    // Request Id & Start-Up Platform Data
    std::string messageA = "e," + ccEvent.getString();
    zmq::message_t messageT1(messageA.size());
    memcpy(messageT1.data(), messageA.data(), messageA.size());
    requester.send(messageT1, 0);

    // Recieve Id & World Start-Up Data
    zmq::message_t messageT2;
    requester.recv(&messageT2, 0);
    std::string stringA = std::string(static_cast<char*>(messageT2.data()), messageT2.size());
    //std::cout << "Id Received: " << string << std::endl;

    // Create World
    world = World(0);

    // Determine Deliminator 
    std::string delimiter = ",";

    // Count Variables 
    std::string stringId;
    int id = -1;
    std::string stringPlankCount;
    int intPlankCount;
    std::string stringStoneWallCount;
    int intStoneWallCount;
    std::string stringLavaCount;
    int intLavaCount;
    std::string stringStratusCount;
    int intStratusCount;
    std::string stringCumulusCount;
    int intCumulusCount;

    // Set Id
    size_t pos = stringA.find(delimiter);
    stringId = stringA.substr(0, stringA.find(delimiter));
    id = std::stoi(stringId);

    // -- Create Planks

    // Find Plank Count
    stringA.erase(0, pos + delimiter.length());
    pos = stringA.find(delimiter);
    stringPlankCount = stringA.substr(0, stringA.find(delimiter));
    intPlankCount = std::stoi(stringPlankCount);

    std::vector<Plank> planks;

    // Add Planks To World
    for (int i = 0; i < intPlankCount; i++) {

        // Plank Variables
        std::string stringPlankId;
        int intPlankId;
        std::string stringPlankX;
        int intPlankX;
        std::string stringPlankY;
        int intPlankY;
        std::string stringPlankLength;
        int intPlankLength;

        // Finding Plank Id
        stringA.erase(0, pos + delimiter.length());
        pos = stringA.find(delimiter);
        stringPlankId = stringA.substr(0, stringA.find(delimiter));
        intPlankId = std::stoi(stringPlankId);

        // Finding Plank X
        stringA.erase(0, pos + delimiter.length());
        pos = stringA.find(delimiter);
        stringPlankX = stringA.substr(0, stringA.find(delimiter));
        intPlankX = std::stoi(stringPlankX);

        // Finding Plank Y
        stringA.erase(0, pos + delimiter.length());
        pos = stringA.find(delimiter);
        stringPlankY = stringA.substr(0, stringA.find(delimiter));
        intPlankY = std::stoi(stringPlankY);

        // Finding Plank Length
        stringA.erase(0, pos + delimiter.length());
        pos = stringA.find(delimiter);
        stringPlankLength = stringA.substr(0, stringA.find(delimiter));
        intPlankLength = std::stoi(stringPlankLength);

        planks.push_back(Plank(intPlankId, intPlankX, intPlankY, intPlankLength));
    }

    // -- Create Stone Walls

    // Find StoneWall Count
    stringA.erase(0, pos + delimiter.length());
    pos = stringA.find(delimiter);
    stringStoneWallCount = stringA.substr(0, stringA.find(delimiter));
    intStoneWallCount = std::stoi(stringStoneWallCount);

    std::vector<StoneWall> stoneWalls;

    // Add Stone Walls To World
    for (int i = 0; i < intStoneWallCount; i++) {

        // Stone Wall Variables
        std::string stringStoneWallId;
        int intStoneWallId;
        std::string stringStoneWallX;
        int intStoneWallX;
        std::string stringStoneWallY;
        int intStoneWallY;
        std::string stringStoneCount;
        int intStoneCount;

        // Finding Stone Wall Id
        stringA.erase(0, pos + delimiter.length());
        pos = stringA.find(delimiter);
        stringStoneWallId = stringA.substr(0, stringA.find(delimiter));
        intStoneWallId = std::stoi(stringStoneWallId);

        // Finding Stone Wall X
        stringA.erase(0, pos + delimiter.length());
        pos = stringA.find(delimiter);
        stringStoneWallX = stringA.substr(0, stringA.find(delimiter));
        intStoneWallX = std::stoi(stringStoneWallX);

        // Finding Stone Wall Y
        stringA.erase(0, pos + delimiter.length());
        pos = stringA.find(delimiter);
        stringStoneWallY = stringA.substr(0, stringA.find(delimiter));
        intStoneWallY = std::stoi(stringStoneWallY);

        // Finding Stone Count
        stringA.erase(0, pos + delimiter.length());
        pos = stringA.find(delimiter);
        stringStoneCount = stringA.substr(0, stringA.find(delimiter));
        intStoneCount = std::stoi(stringStoneCount);

        stoneWalls.push_back(StoneWall(intStoneWallId, intStoneWallX, intStoneWallY, intStoneCount));
    }

    //  -- -- Request Lava Data -- -- \\

    std::string messageB = "New_Lava";
    zmq::message_t messageT1B(messageB.size());
    memcpy(messageT1B.data(), messageB.data(), messageB.size());
    requester.send(messageT1B, 0);

    // Recieve Id & World Start-Up Data
    zmq::message_t messageT2B;
    requester.recv(&messageT2B, 0);
    std::string stringB = std::string(static_cast<char*>(messageT2B.data()), messageT2B.size());
    //std::cout << "Id Received: " << string << std::endl;

    // -- Create Lava 

    // Find Lava Count
    pos = stringB.find(delimiter);
    stringLavaCount = stringB.substr(0, stringB.find(delimiter));
    intLavaCount = std::stoi(stringLavaCount);

    std::vector<Lava> lavas;

    // Add Lavas To World
    for (int i = 0; i < intLavaCount; i++) {

        // Lava Variables
        std::string stringLavaId;
        int intLavaId;
        std::string stringLavaX;
        int intLavaX;
        std::string stringLavaY;
        int intLavaY;
        std::string stringLavaWidth;
        int intLavaWidth;
        std::string stringLavaHeight;
        int intLavaHeight;

        // Finding Lava Id
        stringB.erase(0, pos + delimiter.length());
        pos = stringB.find(delimiter);
        stringLavaId = stringB.substr(0, stringB.find(delimiter));
        intLavaId = std::stoi(stringLavaId);

        // Finding Lava X
        stringB.erase(0, pos + delimiter.length());
        pos = stringB.find(delimiter);
        stringLavaX = stringB.substr(0, stringB.find(delimiter));
        intLavaX = std::stoi(stringLavaX);

        // Finding Lava Y
        stringB.erase(0, pos + delimiter.length());
        pos = stringB.find(delimiter);
        stringLavaY = stringB.substr(0, stringB.find(delimiter));
        intLavaY = std::stoi(stringLavaY);

        // Finding Lava Width
        stringB.erase(0, pos + delimiter.length());
        pos = stringB.find(delimiter);
        stringLavaWidth = stringB.substr(0, stringB.find(delimiter));
        intLavaWidth = std::stoi(stringLavaWidth);

        // Finding Lava Height
        stringB.erase(0, pos + delimiter.length());
        pos = stringB.find(delimiter);
        stringLavaHeight = stringB.substr(0, stringB.find(delimiter));
        intLavaHeight = std::stoi(stringLavaHeight);

        lavas.push_back(Lava(intLavaId, intLavaX, intLavaY, intLavaWidth, intLavaHeight));
    }

    // -- Add Stone Walls, Lava, and Planks -- \\

    for (std::vector<Lava>::iterator it = lavas.begin(); it != lavas.end(); ++it) {
        world.addObject(&(*it));
    }

    for (std::vector<StoneWall>::iterator it = stoneWalls.begin(); it != stoneWalls.end(); ++it) {
        world.addObject(&(*it));
    }

    for (std::vector<Plank>::iterator it = planks.begin(); it != planks.end(); ++it) {
        world.addObject(&(*it));
    }

    //  -- -- Request Cloud Data -- -- \\

    std::string messageC = "New_Clouds";
    zmq::message_t messageT1C(messageC.size());
    memcpy(messageT1C.data(), messageC.data(), messageC.size());
    requester.send(messageT1C, 0);

    // Recieve Id & World Start-Up Data
    zmq::message_t messageT2C;
    requester.recv(&messageT2C, 0);
    std::string stringC = std::string(static_cast<char*>(messageT2C.data()), messageT2C.size());
    //std::cout << "Id Received: " << string << std::endl;

    // -- Create & Add Stratus Clouds

    // Find Stratus Count
    pos = stringC.find(delimiter);
    stringStratusCount = stringC.substr(0, stringC.find(delimiter));
    intStratusCount = std::stoi(stringStratusCount);

    std::vector<StratusCloud> stratusClouds;

    // Add Stratus To World
    for (int i = 0; i < intStratusCount; i++) {

        // Stratus Cloud Variables
        std::string stringStratusId;
        int intStratusId;
        std::string stringStratusX;
        int intStratusX;
        std::string stringStratusY;
        int intStratusY;

        // Finding Stratus Id
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringStratusId = stringC.substr(0, stringB.find(delimiter));
        intStratusId = std::stoi(stringStratusId);

        // Finding Stratus X
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringStratusX = stringC.substr(0, stringC.find(delimiter));
        intStratusX = std::stoi(stringStratusX);

        // Finding Stratus Y
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringStratusY = stringC.substr(0, stringC.find(delimiter));
        intStratusY = std::stoi(stringStratusY);

        stratusClouds.push_back(StratusCloud(intStratusId, intStratusX, intStratusY));
    }

    for (std::vector<StratusCloud>::iterator it = stratusClouds.begin(); it != stratusClouds.end(); ++it) {
        world.addStratusCloud(&(*it));
    }

    // -- Create & Add Culumbus Clouds 

    // Find Culumbus Count
    stringC.erase(0, pos + delimiter.length());
    pos = stringC.find(delimiter);
    stringCumulusCount = stringC.substr(0, stringC.find(delimiter));
    intCumulusCount = std::stoi(stringCumulusCount);

    std::vector<CumulusCloud> cumulusClouds;

    // Add Stratus To World
    for (int i = 0; i < intCumulusCount; i++) {

        // Stratus Cloud Variables
        std::string stringCumulusId;
        int intCumulusId;
        std::string stringCumulusX;
        int intCumulusX;
        std::string stringCumulusY;
        int intCumulusY;
        std::string stringCumulusDistance;
        int intCumulusDistance;

        // Finding Lava Id
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringCumulusId = stringC.substr(0, stringB.find(delimiter));
        intCumulusId = std::stoi(stringCumulusId);

        // Finding Lava X
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringCumulusX = stringC.substr(0, stringC.find(delimiter));
        intCumulusX = std::stoi(stringCumulusX);

        // Finding Lava Y
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringCumulusY = stringC.substr(0, stringC.find(delimiter));
        intCumulusY = std::stoi(stringCumulusY);

        // Finding Distance
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringCumulusDistance = stringC.substr(0, stringC.find(delimiter));
        intCumulusDistance = std::stoi(stringCumulusDistance);

        cumulusClouds.push_back(CumulusCloud(intCumulusId, intCumulusX, intCumulusY, intCumulusDistance));
    }

    for (std::vector<CumulusCloud>::iterator it = cumulusClouds.begin(); it != cumulusClouds.end(); ++it) {
        world.addCumulusCloud(&(*it));
    }

    // -- Create Spawn Points, Death Zone, Boundaries, & Main Avatar
    int idCounter = 100;
    JumpingFallingMovementManager newMovementManager(7, 0.01, 5, 0.125);
    int action;
    Character mainChar(idCounter, &newMovementManager, &action);
    idCounter++;
    SpawnPoint spawn1(idCounter, 150, 100, 200);
    idCounter++;
    SpawnPoint spawn2(idCounter, 1400, 100, -1000);
    idCounter++;
    Boundary rightBoundary(idCounter, 600, 0, true);
    idCounter++;
    Boundary leftBoundary(idCounter, 100, 0, false);
    idCounter++;
    DeathZone deathZone1(idCounter, 2460, 500, 2000, 100);
    idCounter++;
    DeathZone deathZone2(idCounter, 1140, 550, 400, 100);
    idCounter++;
    world.addAvatar(&mainChar);
    world.addSpawnPoint(&spawn1);
    world.addSpawnPoint(&spawn2);
    world.addBoundary(&rightBoundary);
    world.addBoundary(&leftBoundary);
    world.addObject(&deathZone1);
    world.addObject(&deathZone2);

    //world.killAvatar();

    RealTime realTime(1.0);
    GameTime gameTime(1.0, &realTime);
    int ticCount = 0;
    world.addGameTime(&gameTime);
    world.addTic(&ticCount);

    /// ----- Set Up Event Handlers & Manager ----- \\\
    
    CharacterEventHandler charEH(idCounter);
    idCounter++;
    WorldEventHandler worldEH(idCounter);
    idCounter++;

    // Event Manager
    // EventManager eventManager{};

    // Register Collision Handlers
    eventManager.registerHandler(CHARACTER_COLLISION, &charEH);
    eventManager.registerHandler(CHARACTER_COLLISION, &worldEH);

    // Register Jump Handlers
    eventManager.registerHandler(CHARACTER_JUMP, &charEH);
    eventManager.registerHandler(CHARACTER_JUMP, &worldEH);

    // Register Fall Handlers
    eventManager.registerHandler(CHARACTER_FALL, &charEH);
    eventManager.registerHandler(CHARACTER_FALL, &worldEH);

    // Register Death Handlers
    eventManager.registerHandler(CHARACTER_DEATH, &charEH);
    eventManager.registerHandler(CHARACTER_DEATH, &worldEH);

    // Register Spawn Handlers
    eventManager.registerHandler(CHARACTER_SPAWN, &charEH);
    eventManager.registerHandler(CHARACTER_SPAWN, &worldEH);

    // Register User Handlers
    eventManager.registerHandler(USER_INPUT, &charEH);
    eventManager.registerHandler(USER_INPUT, &worldEH);

    // Register Start Recording Handler
    eventManager.registerHandler(START_REC, &worldEH);

    // Register End Recording Handler
    eventManager.registerHandler(END_REC, &worldEH);

    // Register Client Connect Handler 
    eventManager.registerHandler(CLIENT_CONNECT, &worldEH);

    /// ----- Variables For Communicating W/ The Server ----- \\\
    
    //Avatar Coord For Server 
    int x = world.getAvatarCoordX();
    int y = world.getAvatarCoordY();

    int intClients = 1;

    //  Server Message 
    std::string stringD = "";

    // Other Char Array
    std::vector<int> otherCharacters;

    int speedPressed = false;

    /// ----- Spawn Character to Start ----- \\\

    // Retrieve Spawn Stats
    std::vector<int> stats = world.getSpawnStats();

    // Create Translate Variant
    struct variant spawn_translate;
    spawn_translate.mType = variant::TYPE_TNTEGER;
    spawn_translate.m_asInteger = stats.at(0);

    // Create Coord X Variant 
    struct variant spawn_x;
    spawn_x.mType = variant::TYPE_TNTEGER;
    spawn_x.m_asInteger = stats.at(1);

    // Create Coord Y Variant 
    struct variant spawn_y;
    spawn_y.mType = variant::TYPE_TNTEGER;
    spawn_y.m_asInteger = stats.at(2);

    // Create & Raise Event
    std::unordered_map<std::string, variant> spawn_args;
    spawn_args["translate"] = spawn_translate;
    spawn_args["coord_x"] = spawn_x;
    spawn_args["coord_y"] = spawn_y;
    Event spawnEvent(CHARACTER_SPAWN, spawn_args, *world.getTic(), 0);
    eventManager.raiseEvent(spawnEvent);

    ScriptManager scripter = ScriptManager();
    dukglue_register_function(scripter.getContext(), &printMessage, "printMessage");
    dukglue_register_function(scripter.getContext(), &isJumpValid, "isJumpValid");
    dukglue_register_function(scripter.getContext(), &createRaiseEvent, "createRaiseEvent");
    dukglue_register_function(scripter.getContext(), &isRightValid, "isRightValid");
    dukglue_register_function(scripter.getContext(), &isLeftValid, "isLeftValid");
    dukglue_register_function(scripter.getContext(), &getRightAction, "getRightAction");
    dukglue_register_function(scripter.getContext(), &getLeftAction, "getLeftAction");

    std::cout << "-------------- Button Presses --------------" << std::endl;
    std::cout << "LEFT ARROW - MOVE LEFT" << std::endl;
    std::cout << "RIGHT ARROW - MOVE RIGHT" << std::endl;
    std::cout << "UP ARROW - JUMP" << std::endl;
    std::cout << "--------------------------------------------" << std::endl;

    // Set-Up Window
    sf::RenderWindow window(sf::VideoMode(x_max, y_max), "Homework5 - Client1", sf::Style::Close | sf::Style::Resize);
    window.setKeyRepeatEnabled(false);

    while (window.isOpen()) {

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear(sf::Color::Black);

        /** //                                           \\ **\
         *                                                   *
         * ---------------- MAIN LOOP  --------------------- *
         *                                                   *
         *///                                             \\\*

         // Communicate w/ Server 

         // Send Id & Character Data
        std::string messageD = std::to_string(id) + "," + std::to_string(x) + "," + std::to_string(y);
        //std::cout << "Sending -- " << messageD << std::endl;
        zmq::message_t messageT1D(messageD.size());
        memcpy(messageT1D.data(), messageD.data(), messageD.size());
        requester.send(messageT1D, 0);

        // Recieve Update
        zmq::message_t messageT2D;
        requester.recv(&messageT2D, 0);
        stringD = std::string(static_cast<char*>(messageT2D.data()), messageT2D.size());
        //std::cout << "Received: " << stringD << std::endl;

        // Clients 
        pos = stringD.find(delimiter);
        std::string stringClients = stringD.substr(0, stringD.find(delimiter));
        intClients = std::stoi(stringClients);

        // Multiples Clients
        if (intClients > 1) {

            otherCharacters.clear();

            // Update  Other Characters
            for (int i = 1; i < intClients; i++) {

                stringD.erase(0, pos + delimiter.length());
                pos = stringD.find(delimiter);
                std::string clientX = stringD.substr(0, stringD.find(delimiter));
                int intClientX = std::stoi(clientX);

                stringD.erase(0, pos + delimiter.length());
                pos = stringD.find(delimiter);
                std::string clientY = stringD.substr(0, stringD.find(delimiter));
                int intClientY = std::stoi(clientY);

                otherCharacters.push_back(intClientX);
                otherCharacters.push_back(intClientY);

            }

        }

        // Render To Window
        window.clear(sf::Color::Black);

        /** //                                           \\ **\
         *                                                   *
         * ---------------- COUNT TIME  ---------------------*
         *                                                   *
         *///                                             \\\*

        if (*world.getTic() < world.getGameTime()->getTime()) {
            // std::cout << "MAIN LOOP" << std::endl;

            bool validJump = false;

            // Update Stratus Cloud Position
            std::vector<StratusCloud*> stratusVec = world.getStratusClouds();
            for (std::vector<StratusCloud*>::iterator it = stratusVec.begin(); it != stratusVec.end(); ++it) {

                std::string scStringX;
                std::string scStringY;
                int scX;
                int scY;

                stringD.erase(0, pos + delimiter.length());
                pos = stringD.find(delimiter);
                scStringX = stringD.substr(0, stringD.find(delimiter));
                scX = std::stoi(scStringX);

                stringD.erase(0, pos + delimiter.length());
                pos = stringD.find(delimiter);
                scStringY = stringD.substr(0, stringD.find(delimiter));
                scY = std::stoi(scStringY);

                (*it)->setCoordinates(scX, scY);
            }

            // Update Columbus Cloud Position
            std::vector<CumulusCloud*> cumulusVec = world.getCumulusClouds();
            for (std::vector<CumulusCloud*>::iterator it = cumulusVec.begin(); it != cumulusVec.end(); ++it) {

                std::string ccStringX;
                std::string ccStringY;
                int ccX;
                int ccY;

                stringD.erase(0, pos + delimiter.length());
                pos = stringD.find(delimiter);
                ccStringX = stringD.substr(0, stringD.find(delimiter));
                ccX = std::stoi(ccStringX);

                stringD.erase(0, pos + delimiter.length());
                pos = stringD.find(delimiter);
                ccStringY = stringD.substr(0, stringD.find(delimiter));
                ccY = std::stoi(ccStringY);

                (*it)->setCoordinates(ccX, ccY);
            }

            // Update X & Y w/ Avatar Coord
            x = world.getAvatarCoordX();
            y = world.getAvatarCoordY();

            // Respond To Keyboard Input
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && window.hasFocus()) {

                // reactToKeyboard
                scripter.loadScript("scriptFile.js");
                scripter.runScript("reactToKeyboard", 1, 1, 2);

            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && window.hasFocus()) {

                // reactToKeyboard
                scripter.loadScript("scriptFile.js");
                scripter.runScript("reactToKeyboard", 1, 1, 0);

            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && window.hasFocus()) {

                // reactToKeyboard
                scripter.loadScript("scriptFile.js");
                scripter.runScript("reactToKeyboard", 1, 1, 1);

            }

            /// -------- CHECK FOR AUTOMATIC EVENTS ------- \\\
            
            // Has The Character Collided w/ A Stratus Cloud?
            if (world.stratusCollision()) {

                // Create & Raise Event 
                struct variant sc_action;
                sc_action.mType = variant::TYPE_TNTEGER;
                sc_action.m_asInteger = UP;
                std::unordered_map<std::string, variant> sc_args;
                sc_args["action"] = sc_action;
                Event scEvent(CHARACTER_COLLISION, sc_args, *world.getTic(), 5);
                eventManager.raiseEvent(scEvent);

            }
            else if (validJump == false) {
                // Retrieve Other Character Stats ...
                std::vector<int> updateInfo = world.checkCharacterUpdate();

                // Has The Character Died?
                if (updateInfo.at(2) == DIE) {

                    // "Kill" Avatar

                    // Create Event 
                    std::unordered_map<std::string, variant> die_args;
                    Event dieEvent(CHARACTER_DEATH, die_args, *world.getTic(), 5);

                    // Event Handlers React 
                    eventManager.raiseEvent(dieEvent);
                    // charEH.onEvent(dieEvent, &world);
                    // worldEH.onEvent(dieEvent, &world);

                    // Spawn Character & Translate World

                    // Retrieve Spawn Stats
                    std::vector<int> stats = world.getSpawnStats();

                    // Create Translate Variant
                    struct variant spawn_translate;
                    spawn_translate.mType = variant::TYPE_TNTEGER;
                    spawn_translate.m_asInteger = stats.at(0);

                    // Create Coord X Variant 
                    struct variant spawn_x;
                    spawn_x.mType = variant::TYPE_TNTEGER;
                    spawn_x.m_asInteger = stats.at(1);

                    // Create Coord Y Variant 
                    struct variant spawn_y;
                    spawn_y.mType = variant::TYPE_TNTEGER;
                    spawn_y.m_asInteger = stats.at(2);

                    // Create & Raise Event
                    std::unordered_map<std::string, variant> spawn_args;
                    spawn_args["translate"] = spawn_translate;
                    spawn_args["coord_x"] = spawn_x;
                    spawn_args["coord_y"] = spawn_y;
                    Event spawnEvent(CHARACTER_SPAWN, spawn_args, *world.getTic(), 6);
                    eventManager.raiseEvent(spawnEvent);

                }

                // Is The Character Falling Due To A Lack Of A Platform / Collision?
                else if (updateInfo.at(2) == FALL && updateInfo.at(0) == 0) {

                    // Create & Raise Event 
                    std::unordered_map<std::string, variant> fall_args;
                    Event fallEvent(CHARACTER_FALL, fall_args, *world.getTic(), 5);
                    eventManager.raiseEvent(fallEvent);
                }

                // Has The Character Collided w/ Something?
                else if (updateInfo.at(0) == 1) {

                    // Action Varient 
                    struct variant coll_action;
                    coll_action.mType = variant::TYPE_TNTEGER;
                    coll_action.m_asInteger = updateInfo.at(2);

                    // Action Varient 
                    struct variant coll_auto;
                    coll_auto.mType = variant::TYPE_TNTEGER;
                    coll_auto.m_asInteger = 1; // true

                    // Create & Raise Event 
                    std::unordered_map<std::string, variant> coll_args;
                    coll_args["action"] = coll_action;
                    coll_args["auto"] = coll_auto; // true
                    Event collEvent(CHARACTER_COLLISION, coll_args, *world.getTic(), 5);
                    eventManager.raiseEvent(collEvent);

                }

                // Is The Character In A State Of Jumping?
                else if (world.getAvatar()->getJump()) {

                    // Create & Raise Event  
                    std::unordered_map<std::string, variant> jump_args;
                    Event jumpEvent(CHARACTER_JUMP, jump_args, *world.getTic(), 5);
                    eventManager.raiseEvent(jumpEvent);
                }

            }

            // ---------------------- HANDLE EVENTS ----------------------- \\

            eventManager.handleEvents(&world, *world.getTic());


            // ---------------------- UPDATE TIME ----------------------- \\

            *world.getTic() = world.getGameTime()->getTime();
        }

        // Draw Other Characters
        if (intClients > 1) {

            for (int i = 0; i < otherCharacters.size(); i += 2) {
                int intClientX = otherCharacters[i];
                int intClientY = otherCharacters[i + 1];
                int transform = world.getTransform();
                mainChar.renderOtherCharacter(intClientX, intClientY, window, transform);
            }
        }

        // Draw World Objects
        world.render(window, 0);
        window.display();
    }

    /// --------------------- END MAIN LOOP ------------------------------- \\\

    return 0;
}