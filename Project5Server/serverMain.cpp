
#define _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING  1
#include <SFML/Graphics.hpp>
#include "ScriptManager.h"
#include "dukglue/dukglue.h"
#include <string>
#include <iostream>
#include <unordered_map> 
#include <queue>
// using namespace std;
#include <zmq.hpp>
#include <functional>
#include <string.h>
#include <stdlib.h> 
#ifndef _WIN32
#else
#include <thread>
#include <chrono>
#include <mutex>
#include <cassert>

#define sleep(n)    Sleep(n)
#endif
using namespace std::tr1;
#pragma warning(disable : 4996)

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

    int getId() {
        return this->id;
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
    int collide(std::vector<GameObject*> otherObjects, GameObject* thisObject, int action) {

        //std::cout << "Action " << action << std::endl;
        if (action == STILL || action == FALL) // check below for falling 
        {
            (*(*thisObject).getBody()).move(0, 5);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(0, -5);
            int ret = intersection(otherObjects, myBounds, action);
            return ret;
        }

        if (action == JUMP || action == UP) // check above for jump / Up collision
        {
            (*(*thisObject).getBody()).move(0, -5);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(0, 5);
            return intersection(otherObjects, myBounds, action);
        }

        if (action == RIGHT) // check the right for right collision
        {
            (*(*thisObject).getBody()).move(1, -5);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(-1, 5);
            int ret = intersection(otherObjects, myBounds, action);
            //std::cout << "Right Requested; Result Is- Still(0) Fall(1) RIGHT(4) LEFT(3) - " << ret << std::endl;
            return ret;
        }

        if (action == LEFT) // check left for left collision
        {
            (*(*thisObject).getBody()).move(-1, -5);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(1, 5);
            int ret = intersection(otherObjects, myBounds, action);
            //std::cout << "Right Requested; Result Is- Still(0) Fall(1) RIGHT(4) LEFT(3) - " << ret << std::endl;
            return ret;
        }
    }

private:

    // Checks Fpr Intersections Between Objects
    int intersection(std::vector<GameObject*> otherObjects, sf::FloatRect myBounds, int action) {
        bool scrollRight = false;
        bool scrollLeft = false;
        for (std::vector<GameObject*>::iterator it = otherObjects.begin(); it != otherObjects.end(); ++it) {
            int mat = (*it)->getMaterial();
            sf::Shape* body = (*it)->getBody();
            sf::FloatRect otherBounds = (*body).getGlobalBounds();
            if (myBounds.intersects(otherBounds) == true) {
                if (mat == LETHAL) {
                    //std::cout << "LETHAL DETECTED (1)" << std::endl;
                    return DIE;
                }
                else if (mat == SOLID) {
                    if (action == RIGHT || action == LEFT) {
                        //std::cout << "SOLID DETECTED" << std::endl;
                    }
                    if (action == STILL || action == FALL || action == RIGHT || action == LEFT) {
                        //std::cout << "Hit Solid - Still(0) Fall(1) RIGHT(4) LEFT(3) - " << action << std::endl;
                        return STILL;
                    }
                    else  if (action == JUMP) {
                        return FALL;
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
                        return PUSH;
                    }
                }
            }
        }
        if (action == STILL) {
            //std::cout << "Falling" << std::endl;
            return FALL;
        }
        if (scrollRight) {
            //std::cout << "Right Scroll" << std::endl;
            return SCROLL_RIGHT;
        }
        if (scrollLeft) {
            //std::cout << "Left Scroll" << std::endl;
            return SCROLL_LEFT;
        }
        return action;
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
        this->fallingAccel = 0.02;
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
        int ret[2];
        ret[0] = x;
        ret[1] = y;
        if (action == STILL) {
            falling = false;
            jumping = false;
            currentFallingSpeed = startFallingSpeed;
            currentJumpSpeed = startJumpSpeed;
            return ret;
        }
        if (action == JUMP && !falling && y > 0) {
            currentJumpSpeed -= jumpAccel;
            if (currentJumpSpeed < 0) {
                jumping = false;
                falling = true;
                return ret;
            }
            else {
                ret[1] -= currentJumpSpeed;
                return ret;
            }
        }
        if (action == JUMP && falling || action == JUMP && y < 10) {
            jumping = false;
            falling = true;
            currentFallingSpeed += fallingAccel;
            ret[1] += currentFallingSpeed;
            return ret;
        }
        if (action == FALL) {
            currentFallingSpeed += fallingAccel;
            ret[1] += currentFallingSpeed;
            return ret;
        }
        return ret;
    }

    // Returns Whether The Object Is Jumping Or Falling or Neither
    int status() {
        if (jumping) {
            return JUMPING;
        }
        else if (falling) {
            return FALLING;
        }
        else {
            return NONE;
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
    /** Wear A Textured Skin */
    /**
     * https://www.youtube.com/watch?v=NGnjDIOGp8s (loading textures )
     */
    void wearSkin(sf::Shape* body, std::string stringSkin, int red, int green, int blue) {
        sf::Texture texture;
        if (texture.loadFromFile(stringSkin))
        {
            std::cout << "ERROR LOADING IMAGE" << std::endl;
        }
        (*body).setTexture(&texture);
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

  /** Plank Object */
class Plank : public GameObject
{
public:

    Plank() {
        // Plank Default Constructor
    }

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

    int getLength() {
        return length;
    }

private:
    ColorSkinProvider* skinProvider;
    sf::RectangleShape body;
    int length;
};

/** Stone Wall Object */
class StoneWall : public GameObject
{
public:

    StoneWall() {
        // StoneWall Default Constructor
    }

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

    int getStoneCount() {
        return stoneCount;
    }

private:
    std::vector<int> xCoords;
    std::vector<int> yCoords;
    sf::RectangleShape body;
    std::vector<sf::RectangleShape> bodyCollection;
    int stoneCount;
};

/** Lava Class */
class Lava : public GameObject
{
public:
    Lava() {
        // Lava Default Constructor 
    }

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

    // Return Width
    int getWidth() {
        return width;
    }

    // Return Height
    int getHeight() {
        return height;
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

    CumulusCloud() {

    }

    CumulusCloud(int id, int startX, int startY, int distance) {

        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = startX;
        this->coordY = startY;

        this->startX = startX;
        this->startY = startY;
        this->endX = distance + startX - 150;
        this->endY = startY;
        this->direction = 0;

        // Set Material & Body Type
        this->material = SOLID;
        this->bodyType = SINGLE_SHAPE;

        this->action = RIGHT;

        // Set Width & Height and Initalize Body
        sf::RectangleShape rectangle(sf::Vector2f(150, 75));
        this->body = rectangle;

        this->distance = distance;

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

    int getDistance() {
        return distance;
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

    // Updates The Position Of The Moving Platform
    void updatePosition() {
        int* coords = movementManager.nextPosition(coordX, coordY);
        this->coordX = coords[0];
        this->coordY = coords[1]; 
    }

    int getStartX() {
        return this->startX;
    }

    int getStartY() {
        return this->startY;
    }

    int getEndX() {
        return this->endX;
    }

    int getEndY() {
        return this->endY;
    }

    int getDirection() {
        return this->direction;
    }

    void setDirection(int direction) {
        this->direction = direction;
    }

private:

    HorizontalSliderMovementManager movementManager;
    sf::RectangleShape body;
    int width;
    int height;
    int distance;
    int startX;
    int startY;
    int endX;
    int endY;
    int direction;
};

/** Stratus Cloud Class */
class StratusCloud : public GameObject
{
public:
    StratusCloud() {

    }

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
        updatePosition();
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

    // Returns The Action Of The Platform
    int getAction() {
        this->action = movementManager.getStatus();
        return this->action;
    }

    // Returns  Whether This Platform Is Pushing The Provided Items
    int push(std::vector<GameObject*> otherObjects) {
        int action = (*this->collisionDetector).collide(otherObjects, this, JUMP);
        if (action == PUSH) {
            return PUSH;
        }
        return STILL;
    }

    // Update The Stratus CLoud Position In Association With 
    void updatePosition() {
        int* coords = movementManager.nextPosition(coordX, coordY);
        this->coordX = coords[0];
        this->coordY = coords[1];
    }

private:

    VerticalSliderMovementManager movementManager;
    sf::RectangleShape body;
    int width;
    int height;
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
 * * * * * * * * * * * * *
 *                       *
 *  EVENTS END           *
 *                       *
 * * * * * * * * * * * * *
*/



// // // --- Global Variables --- // // //

/** For Accessing Shared Data */

int globalId = 0;
std::mutex g_i_mutex;

/** The Scripter */

ScriptManager scripter;

/** For Managing Clients */

// client data = < id, connected (0 = false, 1 = true), x, y)
std::vector<std::array<int, 4>> clientData;
// time stamp = time client last connected 
std::vector<std::chrono::time_point<std::chrono::system_clock>> clientTime;
int clients = 0;
int roundNum = 1;

/** World Objects */

// Planks
int plankCount;
Plank plank1;
Plank plank2;
Plank plank3;
Plank plank4;

// Stone Walls
int stoneWallCount;
StoneWall stoneWall1;
StoneWall stoneWall2;
StoneWall stoneWall3;
StoneWall stoneWall4;

// Lavas
int lavaCount;
Lava lava1;
Lava lava2;

// Cumulus Cloud
int cumulusCount;
CumulusCloud cumulusCloud1;
CumulusCloud cumulusCloud2;

// Stratus Cloud
int stratusCount;
StratusCloud sCloud1;

/**
 * * * * * * * * * * * * * * * * * * * * * *
 *                                         *
 *  SCRIPTING SPECIFIC FUNCTIONS START     *
 *                                         *
 * * * * * * * * * * * * * * * * * * * * * *
*/


void setCumulusCloudDirection(int id, int direction) {
    if (id == cumulusCloud1.getId()) {
        cumulusCloud1.setDirection(direction);
    }
    else if (id == cumulusCloud2.getId()) {
        cumulusCloud2.setDirection(direction);
    }
}

void setCumulusCloudCoords(int id, int x, int y) {
    if (id == cumulusCloud1.getId()) {
        cumulusCloud1.setCoordinates(x, y);
    }
    else if (id == cumulusCloud2.getId()) {
        cumulusCloud2.setCoordinates(x, y);
    }
}

/**
 * * * * * * * * * * * * * * * * * * * * * *
 *                                         *
 *  SCRIPTING SPECIFIC FUNCTIONS END       *
 *                                         *
 * * * * * * * * * * * * * * * * * * * * * *
*/


void updateClientData(int id, int x, int y) {
    auto currentTime = std::chrono::system_clock::now();
    std::vector<std::chrono::time_point<std::chrono::system_clock>>::iterator it1 = clientTime.begin();

    for (std::vector<std::array<int, 4>>::iterator it2 = clientData.begin(); it2 != clientData.end(); ++it2) {
        if ((*it2).at(0) == id) {
            (*it2).at(1) = 1;
            (*it2).at(2) = x;
            (*it2).at(3) = y;
            (*it1) = currentTime;
        }
        // Also Make Sure Time Is Valid!!!
        else {
            std::chrono::time_point<std::chrono::system_clock> clientTime = (*it1);
            auto secDiff = std::chrono::duration_cast<std::chrono::seconds>(currentTime - clientTime).count();
            if (secDiff > 5 && (*it2).at(1) == 1) {
                (*it2).at(1) = 0;
                clients--;
            }
        }
        ++it1;
    }
}

std::string recieveClientString(int id) {
    std::string str1 = std::to_string(clients);
    for (std::vector<std::array<int, 4>>::iterator it = clientData.begin(); it != clientData.end(); ++it) {
        if ((*it).at(1) == 1 && (*it).at(0) != id) { // If Connected Add To String
            std::string str2 = "," + std::to_string((*it).at(2)) + "," + std::to_string((*it).at(3));
            str1 += str2;
        }
    }
    return str1;
}

/**
* Refferred To The Following Tutorial To Assist With Multithreading Implementation:
* http://thisthread.blogspot.com/2011/08/multithreading-with-zeromq.html
* https://zguide.zeromq.org/docs/chapter2/
*/
void doWork(zmq::context_t& context)
{
    try
    {
        zmq::socket_t socket(context, ZMQ_REP); // 1
        socket.connect("inproc://workers"); // 2
        while (true)
        {
            zmq::message_t request;
            socket.recv(&request);
            std::string data((char*)request.data(), (char*)request.data() + request.size());

            sleep(1);

            bool isConnectEvent = false;
            if (data.size() > 2 && data.substr(0, 2).compare("e,") == 0) {
                Event event(data.substr(2));
                //std::cout << "event type " << std::to_string(event.getType()) << std::endl;
                //std::cout << "client connect " << std::to_string(CLIENT_CONNECT) << std::endl;
                if (event.getType() == CLIENT_CONNECT) {
                    isConnectEvent = true;
                }
            }

            if (isConnectEvent) {
                {
                    // Lock Scope
                    const std::lock_guard<std::mutex> lock(g_i_mutex);
                    //std::cout << "(2) Cloud1.X = " << cumulusCloud1.getCoordX() << std::endl;
                    // Create Message ->

                    // Add Id:
                    std::string message = std::to_string(globalId) + ",";

                    // Add Plank Count & Plank Data
                    message += std::to_string(plankCount) + ",";
                    message += std::to_string(plank1.getId()) + "," + std::to_string(plank1.getCoordX())
                        + "," + std::to_string(plank1.getCoordY()) + "," + std::to_string(plank1.getLength()) + ",";
                    message += std::to_string(plank2.getId()) + "," + std::to_string(plank2.getCoordX())
                        + "," + std::to_string(plank2.getCoordY()) + "," + std::to_string(plank2.getLength()) + ",";
                    message += std::to_string(plank3.getId()) + "," + std::to_string(plank3.getCoordX())
                        + "," + std::to_string(plank3.getCoordY()) + "," + std::to_string(plank3.getLength()) + ",";
                    message += std::to_string(plank4.getId()) + "," + std::to_string(plank4.getCoordX())
                        + "," + std::to_string(plank4.getCoordY()) + "," + std::to_string(plank4.getLength()) + ",";

                    // Add Stone Wall Count & Stone Wall Data
                    message += std::to_string(stoneWallCount) + ",";
                    message += std::to_string(stoneWall1.getId()) + "," + std::to_string(stoneWall1.getCoordX())
                        + "," + std::to_string(stoneWall1.getCoordY()) + "," + std::to_string(stoneWall1.getStoneCount()) + ",";
                    message += std::to_string(stoneWall2.getId()) + "," + std::to_string(stoneWall2.getCoordX())
                        + "," + std::to_string(stoneWall2.getCoordY()) + "," + std::to_string(stoneWall2.getStoneCount()) + ",";
                    message += std::to_string(stoneWall3.getId()) + "," + std::to_string(stoneWall3.getCoordX())
                        + "," + std::to_string(stoneWall3.getCoordY()) + "," + std::to_string(stoneWall3.getStoneCount()) + ",";
                    message += std::to_string(stoneWall4.getId()) + "," + std::to_string(stoneWall4.getCoordX())
                        + "," + std::to_string(stoneWall4.getCoordY()) + "," + std::to_string(stoneWall4.getStoneCount()) + ",";

                    // Store in "messege_t" Object
                    zmq::message_t reply(message.size());
                    memcpy(reply.data(), message.data(), message.size());

                    // Update Client Array
                    std::array<int, 4> arr;
                    arr.at(0) = globalId; // id
                    arr.at(1) = 0; // connected boolean
                    arr.at(2) = 0; // x 
                    arr.at(3) = 0; // y
                    clientData.push_back(arr);

                    auto currentTime = std::chrono::system_clock::now();
                    clientTime.push_back(currentTime);

                    // Increment Id & Client Count
                    globalId++;
                    clients++;

                    // Send Data
                    socket.send(reply);
                }
            }
            else if (data == "New_Lava") {

                {
                    // Lock Scope
                    const std::lock_guard<std::mutex> lock(g_i_mutex);

                    // Create Message ->

                    // Add Lava Count & Lava Data
                    std::string message = std::to_string(lavaCount) + ",";
                    message += std::to_string(lava1.getId()) + "," + std::to_string(lava1.getCoordX())
                        + "," + std::to_string(lava1.getCoordY()) + "," + std::to_string(lava1.getWidth())
                        + "," + std::to_string(lava1.getHeight()) + ",";
                    message += std::to_string(lava2.getId()) + "," + std::to_string(lava2.getCoordX())
                        + "," + std::to_string(lava2.getCoordY()) + "," + std::to_string(lava2.getWidth())
                        + "," + std::to_string(lava2.getHeight()) + ",";

                    // Store in "messege_t" Object
                    zmq::message_t reply(message.size());
                    memcpy(reply.data(), message.data(), message.size());

                    // Send Data
                    socket.send(reply);
                }
            }
            else if (data == "New_Clouds") {

                {
                    // Lock Scope
                    const std::lock_guard<std::mutex> lock(g_i_mutex);

                    // Create Message ->

                    // Add Stratus Cloud Count & Data
                    std::string message = std::to_string(stratusCount) + ",";
                    message += std::to_string(sCloud1.getId()) + "," + std::to_string(sCloud1.getCoordX())
                        + "," + std::to_string(sCloud1.getCoordY()) + ",";

                    // Add Cumulus Cloud Count & Data
                    message += std::to_string(cumulusCount) + ",";
                    message += std::to_string(cumulusCloud1.getId()) + "," + std::to_string(cumulusCloud1.getCoordX())
                        + "," + std::to_string(cumulusCloud1.getCoordY()) + "," + std::to_string(cumulusCloud1.getDistance()) + ",";
                    message += std::to_string(cumulusCloud2.getId()) + "," + std::to_string(cumulusCloud2.getCoordX())
                        + "," + std::to_string(cumulusCloud2.getCoordY()) + "," + std::to_string(cumulusCloud2.getDistance()) + ",";

                    // Store in "messege_t" Object
                    zmq::message_t reply(message.size());
                    memcpy(reply.data(), message.data(), message.size());

                    // Send Data
                    socket.send(reply);
                }
            }
            else {

                // Define Parameters
                std::string delimiter = ",";
                std::string stringId;
                int intId;
                std::string stringX;
                int intX;
                std::string stringY;
                int intY;

                // Start Parse String
                size_t pos = data.find(delimiter);

                // Finding Id
                stringId = data.substr(0, data.find(delimiter));
                intId = std::stoi(stringId);
                data.erase(0, pos + delimiter.length());

                // Finding X
                pos = data.find(delimiter);
                stringX = data.substr(0, data.find(delimiter));
                intX = std::stoi(stringX);
                data.erase(0, pos + delimiter.length());

                // Finding Y
                pos = data.find(delimiter);
                stringY = data;
                intY = stoi(stringY);

                {
                    // Lock Scope
                    const std::lock_guard<std::mutex> lock(g_i_mutex);

                    // Update Client Data
                    updateClientData(intId, intX, intY);

                    // std::cout << "Clients Count" << clients << std::endl;

                    if (clients == 1) {
                        roundNum = 1;
                    }

                    if (roundNum >= clients && clients > 1) {
                        // reset round num
                        // std::cout << "Reset Round Num " << std::endl;
                        roundNum = 1;
                    }
                    else if (clients > 1) {
                        // std::cout << "Inc Round Num For Mult.  Clients " << std::endl;
                        roundNum++;
                    }

                    // Update CloudData
                    if (roundNum == clients) {
                        // std::cout << "Cloud Update - Round Num == Clients (AKA 1 Client) " << std::endl;
                        sCloud1.updatePosition();
                        
                        ScriptManager scripter = ScriptManager();
                        dukglue_register_function(scripter.getContext(), &setCumulusCloudDirection, "setCumulusCloudDirection");
                        dukglue_register_function(scripter.getContext(), &setCumulusCloudCoords, "setCumulusCloudCoords");
                        scripter.loadScript("scriptFile.js");
                        scripter.runScript("updateCCloud", 1, 7, cumulusCloud1.getId(), cumulusCloud1.getCoordX(), cumulusCloud1.getCoordY(),
                            cumulusCloud1.getDirection(), cumulusCloud1.getStartX(), cumulusCloud1.getStartY(),
                            cumulusCloud1.getEndX(), cumulusCloud1.getEndY());
                        scripter.runScript("updateCCloud", 1, 7, cumulusCloud2.getId(), cumulusCloud2.getCoordX(), cumulusCloud2.getCoordY(),
                            cumulusCloud2.getDirection(), cumulusCloud2.getStartX(), cumulusCloud2.getStartY(),
                            cumulusCloud2.getEndX(), cumulusCloud2.getEndY());

                    }

                    // Create Message
                    std::string message = recieveClientString(intId) + "," + std::to_string(sCloud1.getCoordX()) + "," +
                        std::to_string(sCloud1.getCoordY()) + "," + std::to_string(cumulusCloud1.getCoordX()) + "," +
                        std::to_string(cumulusCloud1.getCoordY()) + "," + std::to_string(cumulusCloud2.getCoordX()) + "," +
                        std::to_string(cumulusCloud2.getCoordY());
                    zmq::message_t reply(message.size());
                    memcpy(reply.data(), message.data(), message.size());

                    socket.send(reply);
                }
            }
        }
    }
    catch (const zmq::error_t& ze)
    {
        std::cout << "Exception: " << ze.what() << std::endl;
    }
}

/**
* Refferred To The Following Tutorial To Assist With Multithreading Implementation:
* http://thisthread.blogspot.com/2011/08/multithreading-with-zeromq.html
* https://zguide.zeromq.org/docs/chapter2/
*/
int main()
{

    // Start Count
    int idCounter = 0;

    // Initialize Cumulus Clouds
    cumulusCount = 2;
    cumulusCloud1 = CumulusCloud(idCounter, 425, 150, 400);
    idCounter++;
    cumulusCloud2 = CumulusCloud(idCounter, 1450, 150, 400);
    idCounter++;
    //std::cout << "(1) Cloud1.X = " << cumulusCloud1.getCoordX() << std::endl;

    // Initializes Planks
    plankCount = 4;
    idCounter++;
    plank1 = Plank(idCounter, 20, 450, 350);
    idCounter++;
    plank2 = Plank(idCounter, 515, 450, 615);
    idCounter++;
    plank3 = Plank(idCounter, 1190, 350, 300);
    idCounter++;
    plank4 = Plank(idCounter, 1800, 350, 600);
    idCounter++;

    // Initialize StoneWalls
    stoneWallCount = 4;
    stoneWall1 = StoneWall(idCounter, 20, 0, 9);
    idCounter++;
    stoneWall2 = StoneWall(idCounter, 1135, 350, 5);
    idCounter++;
    stoneWall3 = StoneWall(idCounter, 1495, 350, 5);
    idCounter++;
    stoneWall4 = StoneWall(idCounter, 2410, 350, 5);
    idCounter++;

    // Initialize Lavas
    lavaCount = 2;
    lava1 = Lava(idCounter, 20, 500, 1115, 80);
    idCounter++;
    lava2 = Lava(idCounter, 1500, 400, 900, 180);
    idCounter++;

    // Initialize Stratus Cloud
    stratusCount = 1;
    sCloud1 = StratusCloud(idCounter, 275, 180);
    idCounter++;

    globalId = 0;
    std::thread myThreads[4];

    //std::cout << "(5) Cloud1.X = " << cCloud1.getCoordX() << std::endl;
    try
    {
        zmq::context_t context(1);
        zmq::socket_t clients(context, ZMQ_ROUTER);
        clients.bind("tcp://*:5555");

        //std::cout << "(6) Cloud1.X = " << cCloud1.getCoordX() << std::endl;
        zmq::socket_t workers(context, ZMQ_DEALER);
        zmq_bind(workers, "inproc://workers"); // 2

        //std::cout << "(7) Cloud1.X = " << cCloud1.getCoordX() << std::endl;
        for (int thread_nbr = 0; thread_nbr != 4; thread_nbr++) {
            myThreads[thread_nbr] = std::thread(std::bind(&doWork, std::ref(context)));
        }
        zmq_device(ZMQ_QUEUE, clients, workers);
    }
    catch (const zmq::error_t& ze)
    {
        std::cout << "Exception: " << ze.what() << std::endl;
    }
    for (int i = 0; i < 4; i++) {
        myThreads[i].join();
    }

}