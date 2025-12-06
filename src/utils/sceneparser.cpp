#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>



bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    std::cout << "here's the filepath" << filepath << std::endl;
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    // TODO: Use your Lab 5 code here
    // Task 5: populate renderData with global data, and camera data;
    renderData.globalData = fileReader.getGlobalData();
    renderData.cameraData = fileReader.getCameraData();
    // Task 6: populate renderData's list of primitives and their transforms.
    //         This will involve traversing the scene graph, and we recommend you
    //         create a helper function to do so!

    renderData.shapes.clear();
    renderData.lights.clear();
    SceneNode* root = fileReader.getRootNode();

    parseRecursive(renderData, root, glm::mat4{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});

    return true;
}

void SceneParser::parseRecursive(RenderData &renderData, SceneNode* node, glm::mat4 ctm) {

    if (node == nullptr) return;

    glm::mat4 newctm = ctm;
    for (SceneTransformation* tfm: node->transformations) {
        if (tfm->type == TransformationType::TRANSFORMATION_ROTATE) {
            newctm = newctm * glm::rotate(tfm->angle, tfm->rotate);
        } else if (tfm->type == TransformationType::TRANSFORMATION_SCALE) {
            newctm = newctm * glm::scale(tfm->scale);
        } else if (tfm->type == TransformationType::TRANSFORMATION_TRANSLATE) {
            newctm = newctm * glm::translate(tfm->translate);
        } else if (tfm->type == TransformationType::TRANSFORMATION_MATRIX) {
            newctm = newctm * tfm->matrix;
        }
    }

    for (ScenePrimitive* prim: node->primitives) {
        renderData.shapes.push_back(RenderShapeData{*prim, newctm, glm::inverse(glm::transpose(newctm))});
    }

    for (SceneLight* lit: node->lights) {
        renderData.lights.push_back(SceneLightData{lit->id, lit->type, lit->color, lit->function,
                                                   newctm * glm::vec4{0, 0, 0, 1}, newctm * lit->dir, lit->penumbra, lit->angle, lit->width, lit->height});
    }

    for (SceneNode* child: node->children) {
        parseRecursive(renderData, child, newctm);
    }
}
