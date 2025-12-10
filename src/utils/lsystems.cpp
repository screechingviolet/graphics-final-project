#include "realtime.h"

std::pair<QString, QString> substringInsideAndAfterMatchingBracket(const QString &s) {
    if (s.isEmpty() || s[0] != '[') {
        throw std::runtime_error("String does not start with '['");
    }

    int depth = 0;
    int startInside = 1;  // first character after '['

    for (int i = 0; i < s.length(); i++) {
        if (s[i] == '[') {
            depth++;
        } else if (s[i] == ']') {
            depth--;

            if (depth == 0) {
                // i = index of matching ']'
                QString inside = s.mid(startInside, i - startInside); // substring inside brackets
                QString after  = s.mid(i + 1);                        // substring after brackets
                return { inside, after };
            }
        }
    }

    throw std::runtime_error("Unbalanced brackets");
}

ScenePrimitive defaultPrimitive() {
    ScenePrimitive LSystemBasePrimitive;

    SceneMaterial LSystemGlobalMaterial;
    LSystemGlobalMaterial.cAmbient = glm::vec4(1, 1, 1, 1);
    LSystemGlobalMaterial.cDiffuse = glm::vec4(0.3, 1, 0.3, 1);
    LSystemGlobalMaterial.cSpecular = glm::vec4(0.2, 0.2, 0.2, 1);
    LSystemGlobalMaterial.shininess = 0.4;

    LSystemBasePrimitive.type = PrimitiveType::PRIMITIVE_CYLINDER;
    LSystemBasePrimitive.material = LSystemGlobalMaterial;

    return LSystemBasePrimitive;
}

SceneNode* Realtime::createLSystemNodeHelper(QString data, float localScale, float angle) {
    SceneTransformation *transformationTranslate = new SceneTransformation;
    transformationTranslate->type = TransformationType::TRANSFORMATION_TRANSLATE;
    SceneNode *node = new SceneNode;
    float updatedAngle = angle;
    int currIndex = 0;

    if (data.length() == 0) {
        return node;
    }

    ScenePrimitive *primitive = new ScenePrimitive;
    *primitive = defaultPrimitive();
    node->primitives.push_back(primitive);

    for (int i = 0; i < data.length(); i++) {
        if (data[i] == "F") {
            if (updatedAngle == 0) {
                transformationTranslate->translate = glm::vec3(0.0, m_LSystemScaler * localScale, 0.0);
                node->transformations.push_back(transformationTranslate);
            } else {
                SceneTransformation *transformationRotate = new SceneTransformation;
                transformationRotate->type = TransformationType::TRANSFORMATION_ROTATE;

                //float xRotation = MiscUtilities::randomGen(0, 1);
                float xRotation = 0;
                transformationRotate->rotate = glm::vec3(xRotation, 0.0, 1.0);
                transformationRotate->angle = updatedAngle;
                float translateScale = m_LSystemScaler;

                float horizontalTranslation = 0.5 * translateScale * cos((M_PI / 2) + updatedAngle);
                float verticalTranslation = 0.5 * translateScale * sin((M_PI / 2) - updatedAngle) + 0.5 * translateScale;

                transformationTranslate->translate = glm::vec3(horizontalTranslation, verticalTranslation, 0.0);
                node->transformations.push_back(transformationTranslate);
                node->transformations.push_back(transformationRotate);
            }

            currIndex = i + 1;

            break;
        } else if (data[i] == "-") {
            //updatedAngle -= m_angle + MiscUtilities::randomGen(-0.2, 0.2);
            updatedAngle -= m_angle;
        } else if (data[i] == "+") {
            //updatedAngle += m_angle + MiscUtilities::randomGen(-0.2, 0.2);
            updatedAngle -= m_angle;
        }
    }

    updatedAngle = 0;

    // No need for scaling if uniform size
    if (m_LSystemScaleProgression != 1) {
        SceneTransformation *transformationScale = new SceneTransformation;
        transformationScale->type = TransformationType::TRANSFORMATION_SCALE;
        transformationScale->scale = glm::vec3(m_LSystemScaleProgression, m_LSystemScaleProgression, m_LSystemScaleProgression);
        node->transformations.push_back(transformationScale);
    }

    for (int i = currIndex; i < data.length(); i++) {
        if (data[i] == "F") {
            SceneNode *child = createLSystemNodeHelper(data.mid(i), localScale * m_LSystemScaleProgression, updatedAngle);
            node->children.push_back(child);

            break;
        } else if (data[i] == "[") {
            std::pair<QString, QString> branchStrings = substringInsideAndAfterMatchingBracket(data.mid(i));

            SceneNode *leftChild = createLSystemNodeHelper(branchStrings.first, localScale * m_LSystemScaleProgression, updatedAngle);
            SceneNode *rightChild = createLSystemNodeHelper(branchStrings.second, localScale * m_LSystemScaleProgression, 0);

            node->children.push_back(leftChild);
            node->children.push_back(rightChild);

            break;
        } else if (data[i] == "-") {
            //updatedAngle -= m_angle + MiscUtilities::randomGen(-0.2, 0.2);
            updatedAngle -= m_angle;
        } else if (data[i] == "+") {
            //updatedAngle += m_angle + MiscUtilities::randomGen(-0.2, 0.2);
            updatedAngle += m_angle;
        }
    }

    return node;
}

SceneNode* Realtime::createLSystemNode(QString data) {
    return createLSystemNodeHelper(data, 1, 0);
}

QString Realtime::generateLSystemString(std::map<QChar, QString> rules, QString axiom, int numIterations) {
    QString state = axiom;

    for (int i = 0; i < numIterations; i++) {
        QString newState = "";

        for (QChar c : state) {
            if (rules.find(c) != rules.end()) {
                newState += rules[c];
            } else {
                newState += c;
            }
        }

        state = newState;
    }

    return state;
}
