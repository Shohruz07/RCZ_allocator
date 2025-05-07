#ifndef NODEBOOLTREE_H
#define NODEBOOLTREE_H

#include "BBV.h"
#include "boolinterval.h"
#include "boolequation.h"

// Узел дерева поиска алгоритма DPLL.
// Каждый узел хранит текущее состояние булевого уравнения (eq)
// и указатели на две ветви — lt (присвоение 0) и rt (присвоение 1).
class NodeBoolTree
{
public:
    // Инициализация узла с заданным уравнением
    explicit NodeBoolTree(BoolEquation *equation)
        : eq(equation), lt(nullptr), rt(nullptr)
    {}

    // Копирующий конструктор: копирует только ссылки (не клонирует eq)
    NodeBoolTree(const NodeBoolTree &node)
        : eq(node.eq), lt(node.lt), rt(node.rt)
    {}

    NodeBoolTree *lt;    // левая ветвь (ветвление с присвоением '0')
    NodeBoolTree *rt;    // правая ветвь (ветвление с присвоением '1')

    BoolEquation *eq;    // текущее состояние формулы для этого узла
};

#endif // NODEBOOLTREE_H
