#include "Angel.h"
#include <string.h>

#define COUNT_SUB_CUBE(d) ((d * d * d) - ((d-2) * (d-2) * (d-2)))
#define COUNT_SUB_SIDE_CUBE(d) ((d * d) - ((d-2) * (d-2)))
#define GET_VARIABLE_NAME(Variable) #Variable

#define INITIAL_WINDOW_WIDTH 520
#define INITIAL_WINDOW_HEIGHT 520
#define FRAME_RATE 60
#define CUBE_ROTATION_SPEED 150.0f
#define CUBE_DRAG_SPEED 0.5f
#define SQUARE_EDGE_LEN 0.6f
#define CUBE_DIM 5 // can only be odd numbers 
#define CUBE_SIDE_AREA (CUBE_DIM * CUBE_DIM)
#define CUBE_LEN_AT_DIRECTION (CUBE_DIM-1)/2
#define SUB_CUBE_COUNT      COUNT_SUB_CUBE(CUBE_DIM)
#define SUB_SIDE_CUBE_COUNT COUNT_SUB_SIDE_CUBE(CUBE_DIM)
#define ROTATION_AXIS_DECISION_THRESHOLD 10.0f
#define MAX_INITIAL_ROTATION 15
#define MIN_INITIAL_ROTATION 10

typedef vec4  color4;
typedef vec4  point4;

const color4 SurfaceColors[6] = {color4(1.0, 0.0, 0.0, 1.0), color4(0.0, 1.0, 0.0, 1.0),
                                 color4(0.0, 0.0, 1.0, 1.0), color4(1.0, 1.0, 0.0, 1.0),
                                 color4(1.0, 0.0, 1.0, 1.0), color4(0.0, 1.0, 1.0, 1.0)};

mat4 sub_cube_rotations[SUB_CUBE_COUNT];
int surface_cube_indices[6][CUBE_SIDE_AREA]; // holds the indices of the sub-cubes corresponding to a surface 
                                                  // (-x, x, -y, y, -z, z respectively)

point4 sub_cube_point_coordinates[SUB_CUBE_COUNT];
point4 sub_cube_points[SUB_CUBE_COUNT][36];
color4 sub_cube_colors[SUB_CUBE_COUNT][36];
color4 color_ids[SUB_CUBE_COUNT][36];
point4 center_cube_points[36]; // 36 cube indices are sequential in terms of the
                               // direction of the surfaces (6 indices each): -x, x, -y, y -z, z

int window_dims[2] = {INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT};

int initial_mid_cursor_pos_x, initial_mid_cursor_pos_y; // cursor positions at the time when middle mouse hold starts
bool middle_on_hold = false;

int initial_left_cursor_pos_x, initial_left_cursor_pos_y; // cursor positions at the time when left mouse hold starts
bool left_on_hold = false, passed_rotation_threshold = false, rotation_face_chosen = false;
int dragged_sub_cube_idx, rotation_face_idx, rotation_axis; float rotation_amount;
int sub_cube_indices_to_rotate[CUBE_SIDE_AREA];

GLuint ModelViewsLocation, ProjectionLocation, DrawEdgesLocation, DrawIDLocation;
mat4 ModelViews[SUB_CUBE_COUNT], Projection;

// Vertices of a unit cube centered at origin, sides aligned with axes
point4 vertices[8] = {
    point4( -SQUARE_EDGE_LEN/2, -SQUARE_EDGE_LEN/2,  SQUARE_EDGE_LEN/2, 1.0 ),
    point4( -SQUARE_EDGE_LEN/2,  SQUARE_EDGE_LEN/2,  SQUARE_EDGE_LEN/2, 1.0 ),
    point4(  SQUARE_EDGE_LEN/2,  SQUARE_EDGE_LEN/2,  SQUARE_EDGE_LEN/2, 1.0 ),
    point4(  SQUARE_EDGE_LEN/2, -SQUARE_EDGE_LEN/2,  SQUARE_EDGE_LEN/2, 1.0 ),
    point4( -SQUARE_EDGE_LEN/2, -SQUARE_EDGE_LEN/2, -SQUARE_EDGE_LEN/2, 1.0 ),
    point4( -SQUARE_EDGE_LEN/2,  SQUARE_EDGE_LEN/2, -SQUARE_EDGE_LEN/2, 1.0 ),
    point4(  SQUARE_EDGE_LEN/2,  SQUARE_EDGE_LEN/2, -SQUARE_EDGE_LEN/2, 1.0 ),
    point4(  SQUARE_EDGE_LEN/2, -SQUARE_EDGE_LEN/2, -SQUARE_EDGE_LEN/2, 1.0 )
};

void quad( int a, int b, int c, int d )
{
    static int Index = 0;
    center_cube_points[Index] = vertices[a]; Index++;
    center_cube_points[Index] = vertices[b]; Index++;
    center_cube_points[Index] = vertices[c]; Index++;
    center_cube_points[Index] = vertices[a]; Index++;
    center_cube_points[Index] = vertices[c]; Index++;
    center_cube_points[Index] = vertices[d]; Index++;
}

void center_cube()
{
    quad( 1, 0, 4, 5 );
    quad( 2, 3, 7, 6 );
    quad( 3, 0, 4, 7 );
    quad( 6, 5, 1, 2 );
    quad( 4, 5, 6, 7 );
    quad( 2, 3, 0, 1 );
}

void position_cube(int x, int y, int z, int* *surface_cube_indices_pt = NULL, int sub_cube_index = -1, mat4 RotationMat = RotateX(0))
{
    if(surface_cube_indices_pt != NULL){
        if(x == -CUBE_LEN_AT_DIRECTION)
            *(surface_cube_indices_pt[0]++) = sub_cube_index;
        else if(x == CUBE_LEN_AT_DIRECTION)
            *(surface_cube_indices_pt[1]++) = sub_cube_index;

        if(y == -CUBE_LEN_AT_DIRECTION)
            *(surface_cube_indices_pt[2]++) = sub_cube_index;
        else if(y == CUBE_LEN_AT_DIRECTION)
            *(surface_cube_indices_pt[3]++) = sub_cube_index;

        if(z == -CUBE_LEN_AT_DIRECTION)
            *(surface_cube_indices_pt[4]++) = sub_cube_index;
        else if(z == CUBE_LEN_AT_DIRECTION)
            *(surface_cube_indices_pt[5]++) = sub_cube_index;
    }

    for(int i=0; i<36; i++) {

        if(!surface_cube_indices_pt)
            sub_cube_points[sub_cube_index][i] = RotationMat * sub_cube_points[sub_cube_index][i];

        sub_cube_points[sub_cube_index][i] += point4(x * SQUARE_EDGE_LEN, y * SQUARE_EDGE_LEN, z * SQUARE_EDGE_LEN, 1.0);

        //if(surface_cube_indices_pt == NULL) 
        //    sub_cube_colors[sub_cube_index][i] = SurfaceColors[i/6];
        
    }
        
    sub_cube_point_coordinates[sub_cube_index] = point4(x, y, z, 1.0);
}

void colorize_cubes()
{
    /*for(int i=0; i<SUB_CUBE_COUNT; i++){
        for(int j=0; j<36; j++){
            sub_cube_colors[i][j] = color4(0.0, 0.0, 0.0, 1.0);
        }
    }*/

    for(int surface_idx=0; surface_idx<6; surface_idx++){

        color4 color = SurfaceColors[surface_idx];
        color4 color_id_step = color / CUBE_SIDE_AREA;
        for(int surface_cube_idx=0; surface_cube_idx < CUBE_SIDE_AREA; surface_cube_idx++){
            //std::cout << color << " " << surface_idx << " " << surface_cube_indices[surface_idx][surface_cube_idx] << std::endl;
            for(int i=0; i<6; i++) {
                sub_cube_colors[surface_cube_indices[surface_idx][surface_cube_idx]][6 * surface_idx + i] = color;
                color_ids[surface_cube_indices[surface_idx][surface_cube_idx]][6 * surface_idx + i] = color_id_step * (surface_cube_idx + 1);
            }
        }
    }
}

void randomize_cube_rotations()
{
    int rotation_count = MIN_INITIAL_ROTATION + (rand() % MAX_INITIAL_ROTATION);
    int axis, coordinate_at_axis, rotation_magnitude; mat4 RotationMat;
    for(int rotation=0; rotation<rotation_count; rotation++){
        axis = rand() % 3;
        coordinate_at_axis = (rand() % 3) - 1;
        rotation_magnitude = 90 * (rand() % 4);
        switch(axis){
            case 0:
                RotationMat = RotateX(rotation_magnitude);
                break;
            case 1:
                RotationMat = RotateY(rotation_magnitude);
                break;
            case 2:
                RotationMat = RotateZ(rotation_magnitude);
                break;
        }

        for(int sub_cube_idx=0; sub_cube_idx<SUB_CUBE_COUNT; sub_cube_idx++){
            if(sub_cube_point_coordinates[sub_cube_idx][axis] == coordinate_at_axis) {
                sub_cube_rotations[sub_cube_idx] = RotationMat * sub_cube_rotations[sub_cube_idx];
                for(int i=0; i<36; i++){
                    sub_cube_points[sub_cube_idx][i] = RotationMat * sub_cube_points[sub_cube_idx][i];
                }

                sub_cube_point_coordinates[sub_cube_idx] = RotationMat * sub_cube_point_coordinates[sub_cube_idx];
                for(int i=0; i<4; i++){
                    sub_cube_point_coordinates[sub_cube_idx][i] = round(sub_cube_point_coordinates[sub_cube_idx][i]);
                }
            }
        }
    }

/*
    int rotation_count; mat4 RotationMat;
    for(int sub_cube_idx=0; sub_cube_idx<SUB_CUBE_COUNT; sub_cube_idx++){
        RotationMat = Translate(vec3(0.0, 0.0, 0.0));
        for(int axis=0; axis<3; axis++){
            rotation_count = rand() % 4;
            switch(axis){
                case 0:
                    RotationMat = RotateX(rotation_count * 90) * RotationMat;
                    break;
                case 1:
                    RotationMat = RotateY(rotation_count * 90) * RotationMat;
                    break;
                case 2:
                    RotationMat = RotateZ(rotation_count * 90) * RotationMat;
                    break;
            }
        }
        sub_cube_rotations[sub_cube_idx] = RotationMat;
    }
    */
}

// Create all the sub cubes of the rubiks cube
void sub_cubes()
{
    int sub_cube_index = 0;
    int** surface_cube_indices_pt = (int**) malloc(6 * sizeof(int*));
    for(int i=0; i<6; i++) surface_cube_indices_pt[i] = surface_cube_indices[i];

    for(int x = -CUBE_LEN_AT_DIRECTION; x <= CUBE_LEN_AT_DIRECTION; x++){
        for(int y = -CUBE_LEN_AT_DIRECTION; y <= CUBE_LEN_AT_DIRECTION; y++){
            for(int z = -CUBE_LEN_AT_DIRECTION; z <= CUBE_LEN_AT_DIRECTION; z++){

                // A coordinate which does not include any maximum value for any axis should not be included as a sub-cube
                if(abs(x) != CUBE_LEN_AT_DIRECTION && abs(y) != CUBE_LEN_AT_DIRECTION && abs(z) != CUBE_LEN_AT_DIRECTION)
                    continue;

                // Copy the point values of the central cube, and translate it to get sub-cubes
                memcpy(sub_cube_points[sub_cube_index], center_cube_points, 36 * sizeof(point4));
                position_cube(x, y, z, surface_cube_indices_pt, sub_cube_index);

                // Model Views are identity at the beginning
                ModelViews[sub_cube_index] = Translate(vec3(0.0, 0.0, 0.0));
                sub_cube_rotations[sub_cube_index] = Translate(vec3(0.0, 0.0, 0.0));

                sub_cube_index++;
            }
        }
    }

    randomize_cube_rotations();

    // Give color attribute to every point in the rubiks cube
    colorize_cubes();

    free(surface_cube_indices_pt);
}

void get_all_sub_cube_indices_at_axis(int clicked_sub_cube_index, int rotation_axis) 
{
    int i=0;
    for(int sub_cube_idx=0; sub_cube_idx<SUB_CUBE_COUNT; sub_cube_idx++){
        
        // Round the coordinates to evade any floating point calculation error
        if(round(sub_cube_point_coordinates[sub_cube_idx][rotation_axis] * 100) /100 != 
           round(sub_cube_point_coordinates[clicked_sub_cube_index][rotation_axis] * 100) / 100)
            continue;

        sub_cube_indices_to_rotate[i++] = sub_cube_idx;
    }

    // Indicate the end of array with -1
    sub_cube_indices_to_rotate[i] = -1;
}

// Return the bold version of the string
char* bold(char* str) {
    char* bold_str = (char *) malloc(sizeof(char) * (strlen(str) + 9));
    sprintf(bold_str, "%c[1m%s%c[0m", 27, str, 27);
    return bold_str;
}

// Print the help text
void print_help() {
    printf("\n************ HELP *************\n");
    printf("Use %s to %s the cube\n", bold((char*) "left click"), bold((char*) "rotate"));
    printf("Use %s continuously to %s the view\n", bold((char*) "middle click"), bold((char*) "change"));
    printf("Use %s to %s a face to rotate\n", bold((char*) "right click"), bold((char*) "select"));
    printf("Change the %s constant at the beginning of the code to %s of the cube\n", bold((char*) "CUBE_DIM"), bold((char*) "change the dimensions"));
    printf("*******************************\n\n");
}

void buffer_current_locations()
{
    int i=0, sub_cube_idx;
    while((sub_cube_idx = sub_cube_indices_to_rotate[i++]) != -1){

        // Position the cube again, according to the latest rotations
        vec4 sub_cube_coordinate = sub_cube_point_coordinates[sub_cube_idx];
        memcpy(sub_cube_points[sub_cube_idx], center_cube_points, 36 * sizeof(point4));
        position_cube(sub_cube_coordinate.x, sub_cube_coordinate.y, sub_cube_coordinate.z, NULL, sub_cube_idx, sub_cube_rotations[sub_cube_idx]);

        // Reset the according model views
        ModelViews[sub_cube_idx] = Translate(vec3(0.0, 0.0, 0.0));
        glBufferSubData(GL_UNIFORM_BUFFER, sub_cube_idx * sizeof(mat4), sizeof(mat4), ModelViews[sub_cube_idx]);
    }

    // Buffer the point data
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(sub_cube_points), sub_cube_points );
}

void rotate_sub_cubes(bool is_static = false){

    // Calculate the rotation matrices. An inverse matrix is required because
    // the z-axis is reflected due to the projection
    mat4 RotationMat, RotationMatInv;
    switch(rotation_axis){
        case 0:
            RotationMat = RotateX(rotation_amount);
            RotationMatInv = RotateX(-rotation_amount);
            break;
        case 1:
            RotationMat = RotateY(rotation_amount);
            RotationMatInv = RotateY(-rotation_amount);
            break;
        case 2:
            RotationMat = RotateZ(rotation_amount);
            RotationMatInv = RotateZ(-rotation_amount);
            break;
    }

    int i=0, sub_cube_idx;
    while((sub_cube_idx = sub_cube_indices_to_rotate[i++]) != -1){

        // if the user still holds the left mouse button, only update the Model Views in the buffer
        glBufferSubData(GL_UNIFORM_BUFFER, sub_cube_idx * sizeof(mat4), sizeof(mat4), RotationMatInv * ModelViews[sub_cube_idx]);

        // if left mouse button is released;
        if(is_static) {

            // Calculate the new sub-cube coordinates
            sub_cube_point_coordinates[sub_cube_idx] = RotationMat * sub_cube_point_coordinates[sub_cube_idx];
            for(int i=0; i<3; i++)
                sub_cube_point_coordinates[sub_cube_idx][i] = round(sub_cube_point_coordinates[sub_cube_idx][i]);

            // Calculate the rotations until now
            sub_cube_rotations[sub_cube_idx] = RotationMat * sub_cube_rotations[sub_cube_idx];
            
            // Buffer the new locations of the points
            buffer_current_locations();
        }
    }
}

// This is needed to pass constants from the source code to shader (Look at InitShader for further understanding)
char* stringifyConstant(std::string varName, int varValue) {
	int len = 8 + varName.length() + 10; // 8 for (#define ) and 10 for ( SUB_CUBE_COUNT\n\0)
    char* definitionString = (char*) malloc(len);

    strcpy(definitionString, "#define ");
    strcat(definitionString, varName.c_str()); strcat(definitionString, " ");

    char varValueBuffer[8];
    sprintf(varValueBuffer, "%d", varValue);
    strcat(definitionString, varValueBuffer); strcat(definitionString, "\n");

    return definitionString;
}


void init()
{
    char* stringifiedConstant = stringifyConstant(GET_VARIABLE_NAME(SUB_CUBE_COUNT), SUB_CUBE_COUNT);

    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader.glsl", "fshader.glsl", stringifiedConstant);
    glUseProgram( program );

    free(stringifiedConstant);

    // Generate the central cube and the sub-cubes
    center_cube();
    sub_cubes();

    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );

    glBufferData( GL_ARRAY_BUFFER, sizeof(sub_cube_points) + sizeof(sub_cube_colors) + sizeof(color_ids), NULL, GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(sub_cube_points), sub_cube_points );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(sub_cube_points), sizeof(sub_cube_colors), sub_cube_colors );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(sub_cube_points) + sizeof(sub_cube_colors), sizeof(color_ids), color_ids);

    // set up vertex arrays
    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

    GLuint vColor = glGetAttribLocation( program, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(sub_cube_points)) );

    GLuint vColorID = glGetAttribLocation( program, "vColorID" );
    glEnableVertexAttribArray( vColorID );
    glVertexAttribPointer( vColorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(sub_cube_points) + sizeof(sub_cube_colors)) );

    // Retrieve transformation uniform variable locations
    ProjectionLocation = glGetUniformLocation( program, "Projection" );
    DrawEdgesLocation = glGetUniformLocation( program, "DrawEdges" );
    ModelViewsLocation = glGetUniformLocation( program, "ModelViews" );
    DrawIDLocation = glGetUniformLocation( program, "DrawID" );

    // Initially, set the draw id boolean to false
    glUniform1i( DrawIDLocation, false );

    GLuint modelViewBuffer;
    glGenBuffers(1, &modelViewBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, modelViewBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ModelViews), ModelViews, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, modelViewBuffer);

    // Set projection matrix
    float k = CUBE_LEN_AT_DIRECTION;
    Projection = RotateX(-20.0) * RotateY(45.0) * Ortho(-k, k, -k, k, -k, k);
    glUniformMatrix4fv( ProjectionLocation, 1, GL_TRUE, Projection );

    glEnable( GL_DEPTH_TEST );
    glClearColor( 0.0, 0.0, 0.0, 1.0 );
}

// Used to change the viewving matrix
void handle_middle_click(GLFWwindow* window, int action)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if(action == GLFW_PRESS){
        middle_on_hold = true;
        initial_mid_cursor_pos_x = x;
        initial_mid_cursor_pos_y = y;
    } else if(action == GLFW_RELEASE){
        Projection = RotateX(-CUBE_ROTATION_SPEED * (y - initial_mid_cursor_pos_y) / window_dims[1]) *
                     RotateY(-CUBE_ROTATION_SPEED * (x - initial_mid_cursor_pos_x) / window_dims[0]) * Projection;
        
        glUniformMatrix4fv( ProjectionLocation, 1, GL_TRUE, Projection );
        middle_on_hold = false;
    }
}

void change_face_color(int face_idx, color4 new_color)
{
    /*for(int cube_idx=0; cube_idx<CUBE_SIDE_AREA; cube_idx++){
        for(int point_idx=0; point_idx<6; point_idx++)
            sub_cube_colors[surface_cube_indices[face_idx][cube_idx]][6*face_idx + point_idx] = new_color;
    }*/

    for(int sub_cube_idx=0; sub_cube_idx<SUB_CUBE_COUNT; sub_cube_idx++){
        if(sub_cube_point_coordinates[sub_cube_idx][face_idx/2] != (2*(face_idx%2) - 1))
            continue;
        
        for(int point_idx=0; point_idx<6; point_idx++)
            sub_cube_colors[sub_cube_idx][6*face_idx + point_idx] = new_color;
    }

    glBufferSubData( GL_ARRAY_BUFFER, sizeof(sub_cube_points), sizeof(sub_cube_colors), sub_cube_colors );
}

int pick_sub_cube_idx(GLFWwindow* window, double x, double y, int& face)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
    glUniform1i( DrawEdgesLocation, false );
    glUniform1i( DrawIDLocation, true );
    glDrawArrays( GL_TRIANGLES, 0, sizeof(sub_cube_points) / sizeof(point4) );

    glFlush(); //forces all drawing commands to be sent to the graphics card and executed immediately.
    glFinish();

    //glReadPixels reads from frame buffer, hence use frame buffer size
    unsigned char pixel[4];
    glReadPixels(x, window_dims[1] - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    face = -1;

    // Check whether the outside (a non sub-cube location) is clicked
    int i;
    for(i=0; i<3; i++){
        if(pixel[i] != 0)
            break;
    }

    if(i == 3)
        return -1;

    for(int sub_cube_idx=0; sub_cube_idx<SUB_CUBE_COUNT; sub_cube_idx++){

        int face_idx, point_idx;
        for(face_idx=0; face_idx<6; face_idx++){
            for(point_idx=0; point_idx<6; point_idx++){

                int i;
                for(i=0; i<3; i++){
                    // Check whether colors of every point match
                    if(round(255 * color_ids[sub_cube_idx][6*face_idx + point_idx][i]) != pixel[i])
                        break;
                }

                if(i != 3)
                    break;
            }

            if(point_idx == 6)
                break;
        }

        // If there is a match;
        if(face_idx != 6){

            // Get the correct face index of the picked sub-cube's picked face
            // by rotating the axis vector
            int axis; vec4 face_vect = vec4(0.0, 0.0, 0.0, 1.0);
            face_vect[face_idx/2] = 2*(face_idx % 2) - 1;
            face_vect = sub_cube_rotations[sub_cube_idx] * face_vect;
            for(axis=0; axis<3; axis++){
                if(abs(round(face_vect[axis])) == 1){
                    break;
                }
            }
            face = 2*axis + (face_vect[axis]>0);

            return sub_cube_idx;
        }
    }

    return -1;
}

void handle_left_click(GLFWwindow* window, int action)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if(action == GLFW_PRESS){
        int sub_cube_idx, face_idx;
        sub_cube_idx = pick_sub_cube_idx(window, x, y, face_idx);

        if(sub_cube_idx == -1)
            return;

        dragged_sub_cube_idx = sub_cube_idx;

        left_on_hold = true;
        initial_left_cursor_pos_x = x;
        initial_left_cursor_pos_y = y;
    } else if(action == GLFW_RELEASE){

        if(!left_on_hold)
            return;

        if(passed_rotation_threshold) {
            rotation_amount = round(rotation_amount / 90.0) * 90.0;
            rotate_sub_cubes(true);
            rotation_amount = 0.0;
        }

        //change_face_color(rotation_face_idx, SurfaceColors[rotation_face_idx]);
        rotation_face_chosen = false;
        passed_rotation_threshold = false;
        left_on_hold = false;
    }
}

void handle_right_click(GLFWwindow* window, int action)
{
    if(action != GLFW_PRESS)
        return;

    //if(rotation_face_chosen)
        //change_face_color(rotation_face_idx, SurfaceColors[rotation_face_idx]);

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    int sub_cube_idx, face_idx, location_idx;
    sub_cube_idx = pick_sub_cube_idx(window, x, y, face_idx);

    if(sub_cube_idx == -1) {
        rotation_face_chosen = false;
        return;
    }

    rotation_face_idx = face_idx;
    //change_face_color(face_idx, SurfaceColors[face_idx] / 2);
    rotation_face_chosen = true;
}

void reshape_callback(GLFWwindow* window, int new_width, int new_height) {
    window_dims[0] = new_width;
    window_dims[1] = new_height;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    switch(button){
        case GLFW_MOUSE_BUTTON_MIDDLE:
            handle_middle_click(window, action);
            break;
        case GLFW_MOUSE_BUTTON_LEFT:
            handle_left_click(window, action);
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            handle_right_click(window, action);
            break;
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(middle_on_hold){
        glUniformMatrix4fv( ProjectionLocation, 1, GL_TRUE, 
            RotateX(-CUBE_ROTATION_SPEED * (ypos - initial_mid_cursor_pos_y) / window_dims[1]) *
            RotateY(-CUBE_ROTATION_SPEED * (xpos - initial_mid_cursor_pos_x) / window_dims[0]) * Projection );
    }

    if(left_on_hold){
        vec4 drag_vector = Projection * vec4(xpos - initial_left_cursor_pos_x, initial_left_cursor_pos_y - ypos, 0.0, 0.0);
        
        if(!passed_rotation_threshold){

            // If the length of the vector is not enough to decide, return
            if(length(drag_vector) < ROTATION_AXIS_DECISION_THRESHOLD)
                return;

            if(rotation_face_chosen){
                rotation_axis = rotation_face_idx / 2;
            } else {
                int possible_rotation_axes[3] = {0, 1, 2};

                int force_direction = 0;
                for(int i=1; i<3; i++){
                    if(abs(drag_vector[i]) > abs(drag_vector[force_direction]))
                        force_direction = i;
                }
                possible_rotation_axes[force_direction] = -1;

                vec3 temp = vec3(sub_cube_point_coordinates[dragged_sub_cube_idx].x,
                                 sub_cube_point_coordinates[dragged_sub_cube_idx].y,
                                 sub_cube_point_coordinates[dragged_sub_cube_idx].z);

                temp[force_direction] = 0.0f;
                normalize(temp);
                for(int i=0; i<3; i++){
                    if(temp[i] == 1){
                        possible_rotation_axes[i] = -1;
                        break;
                    }
                }

                for(int i=0; i<3; i++){
                    if(possible_rotation_axes[i] != -1){
                        rotation_axis = i;
                        break;
                    }
                }
            }

            get_all_sub_cube_indices_at_axis(dragged_sub_cube_idx, rotation_axis);
            passed_rotation_threshold = true;
        } else {
            vec4 axis_vector = vec4(0.0, 0.0, 0.0, 0.0);
            vec4 drag_vector_3d = vec4(drag_vector);
            drag_vector_3d.w = 0.0;
            axis_vector[rotation_axis] = 1.0;
            float angle = acos(dot(axis_vector, drag_vector_3d) / (length(axis_vector) * length(drag_vector_3d)));

            rotation_amount = length(drag_vector) * CUBE_DRAG_SPEED;//(2 * (angle > M_PI_2) - 1) * length(drag_vector) * CUBE_DRAG_SPEED;
            rotate_sub_cubes();
        }

    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

    if (action != GLFW_PRESS)
        return;

    switch(key) {
        case GLFW_KEY_H:
            print_help();
            break;
    }
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniform1i( DrawIDLocation, false );
    
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
    glUniform1i( DrawEdgesLocation, false );
    glDrawArrays( GL_TRIANGLES, 0, sizeof(sub_cube_points) / sizeof(point4) );

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
    glUniform1i( DrawEdgesLocation, true );
    glDrawArrays( GL_TRIANGLES, 0, sizeof(sub_cube_points) / sizeof(point4) );    

    glFlush();
}

void update()
{

}

int main()
{
    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, "Rubiks Cube", NULL, NULL);
    glfwMakeContextCurrent(window);

    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetFramebufferSizeCallback(window, reshape_callback);

    print_help();

    glewExperimental = GL_TRUE;
    if(glewInit() != GLEW_OK) {
        std::cout << "glew init failed\n"; 
        exit(EXIT_FAILURE);
    }
    
    srand(time(NULL));
    init();

    double currentTime, previousTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        currentTime = glfwGetTime();
        if (currentTime - previousTime >= 1/FRAME_RATE){
            previousTime = currentTime;
            update();
        }
        
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}