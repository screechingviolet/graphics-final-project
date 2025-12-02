## Project 6: Final Project Gear Up

The project handout can be found [here](https://cs1230.graphics/projects/final/gear-up).

### Test Cases

Insert your test cases here. Be sure to include images/videos as needed!

Each of the following test cases compares the output from this [online glTF viewer](https://gltf-viewer.donmccurdy.com/) with my own outputs. The online one has textures while mine do not, but the movements and geometry should be the same.

Mesh Library-Assisted Loading from glTF/glb Files - 1 star

Test Case 1: Mesh from glb file loads correctly

Test Case 2: Mesh from glb file loads correctly in scene with other primitives/meshes

Test Case 3: Lighting and such effects are still applied correctly (lighting looks correct -> right normals applied)

Kinematic Skeletons + Linear Blend Skinning - 4 stars (2 each)

Test Case 1: Individual model with animation

Test Case 2: Two models with simultaneous different animations

Test Case 3: Mesh with skeleton but no animation loads correctly

Test Case 4: Normals for animated mesh still look right (wasn't sure how to compare this one so I just zoomed in)

### Design Choices

I chose to add a primitive for 'mesh' under shapes/ and implement the class structure in mesh.h for the kinematic skeletons. There is a central Mesh class which offers public functions to load a mesh from filename and update final bone matrices based on animation keyframes (only applicable if hasAnimation is true). 

Then, there is a Bone class with interpolation methods for use between its stored keyframes, which are stored using the KeyframeVec3 (for scaling + translation) and KeyframeQuaternion (for rotation) classes. The Bone class also stores the bone's local transform, its parent, and its inverse bind matrix (m_toBoneSpace).

There are also the Anim and AnimState classes, the first of which stores info that stays the same throughout and the second of which stores the 'current state' of the animation. This is then updated in the Realtime class, in the timerEvent function which adds the time passed to current time and updates the final bone matrices accordingly. These are passed as a uniform to the vertex shader, and the VAO includes the skeleton joints and weights for each vertice, which allows linear blend skinning to be carried out to determine the vertex's final position.

One design choice I made that I found interesting was within the problem of finding the corresponding keyframe to a timestep, in a sorted vector of KeyframeVec3/KeyframeQuaternion. The reference tutorial just iterated through until it was found, but I decided to implement a binary search to make this faster.


### Collaboration/References

All the code for which I used ChatGPT is in the mesh.cpp file. I also used it for miscellaneous advice, particularly about the glTF format. A full transcript of my conversation is [here](https://chatgpt.com/share/691f9d59-c6e0-8001-b488-8917ce285d51), but the main code and assistance it provided consists of the following

- boilerplate for using cgltf, mainly the non-animated lines of Mesh::setVertexData
- explanations of where relevant data is located in cgltf's output
- explanations of the inverse bind and bind matrices, and how to construct the global transform for a joint
- the nature of quaternions in openGL and debugging their differing representation

I also referenced the following resources from the final features list: https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation, https://graphics.cs.cmu.edu/courses/15-466-f17/notes/skinning.html

I used the [cgltf](https://github.com/jkuhlmann/cgltf/blob/master/cgltf.h#L604) header-only library to help with loading the files, and the [glTF documentation](https://github.khronos.org/glTF-Tutorials/) for clarifications. 

I got the models above from [this free pack](https://www.cgtrader.com/free-3d-models/animals/mammal/animals-free). I used Blender to apply the given animations and export as glb.

### Known Bugs

N/A

### Extra Credit

N/A