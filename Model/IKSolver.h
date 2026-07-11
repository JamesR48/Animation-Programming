#ifndef CPPANIMPROGRAMMING_IKSOLVER_H
#define CPPANIMPROGRAMMING_IKSOLVER_H

#include <memory>
#include <vector>
#include <glm/glm.hpp>


class GltfNode;

class IKSolver
{
public:
    IKSolver();
    IKSolver(unsigned int Iterations);
    void SetNodes(std::vector<std::shared_ptr<GltfNode>> Nodes);
    std::shared_ptr<GltfNode> GetIKChainRootNode();

    void SetNumIterations(unsigned int Iterations);

    bool SolveCCD(glm::vec3 Target);
    bool SolveFABRIK(glm::vec3 Target);

private:
    /* nodes from effector (at index 0) to IK chain root node (last index) */
    std::vector<std::shared_ptr<GltfNode>> mNodes{};
    std::vector<float> mBoneLengths{};

    void CalculateBoneLengths();

    void SolveFABRIKForward(glm::vec3 Target);
    void SolveFABRIKBackward(glm::vec3 Base);
    void AdjustFABRIKNodes();

    // Copy of the real Node positions to iterate over
    std::vector<glm::vec3> mFABRIKNodePositions{};

    unsigned int mIterations = 0;
    float mThreshold = 0.00001f;
};

#endif //CPPANIMPROGRAMMING_IKSOLVER_H
