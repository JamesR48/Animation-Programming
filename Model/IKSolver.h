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

private:
    /* nodes from effector (at index 0) to IK chain root node (last index) */
    std::vector<std::shared_ptr<GltfNode>> mNodes{};

    unsigned int mIterations = 0;
    float mThreshold = 0.00001f;
};

#endif //CPPANIMPROGRAMMING_IKSOLVER_H
