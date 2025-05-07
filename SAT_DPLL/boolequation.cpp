#include "boolequation.h"
#include <vector>
#include <algorithm>
#include <ostream>
#include <string>

// Конструктор с параметрами: инициализирует структуру CNF (дизъюнкты), корень, маску и количество актуальных строк
BoolEquation::BoolEquation(BoolInterval **cnf, BoolInterval *root, int cnfSize, int count, BBV mask)
{
    this->cnf = new BoolInterval*[cnfSize];

    for (int i = 0; i < cnfSize; i++) {
        this->cnf[i] = cnf[i];  // копирование указателей на интервалы
    }

    this->root = root;
    this->cnfSize = cnfSize;
    this->count = count;
    this->mask = mask;
}

// Конструктор копирования: копирует все поля, но делает deep copy только для root
BoolEquation::BoolEquation(BoolEquation &equation)
{
    this->cnf = new BoolInterval*[equation.cnfSize];

    for (int i = 0; i < equation.cnfSize; i++) {
        this->cnf[i] = equation.cnf[i];  // shallow copy интервалов (возможно, стоит заменить на глубокое копирование)
    }

    this->root = new BoolInterval(equation.root->vec, equation.root->dnc); // глубокая копия root
    this->cnfSize = equation.cnfSize;
    this->count = equation.count;
    this->mask = equation.mask;
}

// Основная функция проверки и применения правил упрощения
int BoolEquation::CheckRules()
{
    BBV rez, rez1, rez0;
    bool rezInit = false;

    for (int i = 0; i < cnfSize; i++) {
        BoolInterval *interval = cnf[i];

        if (interval != nullptr) {
            // Правило 2: строка пустая (все значения -), тогда формула неразрешима
            if (Rule2RowNull(interval)) {
                return 0;
            }

            // При наличии всего одной строки можно упростить по столбцам сразу
            if (count == 1) {
                if (Rule4Col0(interval->vec ^ interval->dnc)) {
                    return 1;
                }

                if (Rule5Col1(interval->vec)) {
                    return 1;
                }
            }

            // Правило 1: если в строке осталась одна переменная
            if (Rule1Row1(interval)) {
                for (int k = 0; k < interval->length(); k++) {
                    if (mask[k] != 1) {
                        char value = interval->getValue(k);
                        if (value != '-') {
                            Simplify(k, value);  // Применение значения
                            break;
                        }
                    }
                }
                return 1;
            }

            // Вычисление агрегированных векторов для других правил
            if (!rezInit) {
                rez0 = interval->vec ^ interval->dnc; // значения 0
                rez1 = interval->vec;                 // значения 1
                rez  = interval->dnc;                 // don’t care
                rezInit = true;
            } else {
                rez = rez & interval->dnc;
                BBV temprez = interval->vec ^ interval->dnc;
                rez0 = rez0 | temprez;
                rez1 = rez1 & interval->vec;
            }
        }
    }

    // Правило 3: все элементы столбца "неважны" — можно исключить переменную
    Rule3ColNull(rez);

    // Правило 4: все значения столбца — 0 (значение переменной должно быть 0)
    if (Rule4Col0(rez0)) {
        return 1;
    }

    // Правило 5: все значения столбца — 1 (значение переменной должно быть 1)
    if (Rule5Col1(rez1)) {
        return 1;
    }

    return 2; // Ни одно правило не применимо
}

// Правило 2: строка состоит только из don't care и уже известных переменных
bool BoolEquation::Rule2RowNull(BoolInterval *interval)
{
    for (int i = 0; i < mask.getSize(); i++) {
        if (mask[i] != 1 && interval->getValue(i) != '-') {
            return false;
        }
    }
    return true;
}

// Правило 1: строка содержит ровно одну не замаскированную переменную
bool BoolEquation::Rule1Row1(BoolInterval *interval)
{
    int counter = 0;
    for (int i = 0; i < mask.getSize(); i++) {
        if (mask[i] != 1 && interval->getValue(i) != '-') {
            counter++;
        }
    }
    return counter == 1;
}

// Правило 3: все переменные в столбце имеют don't care (или были замаскированы)
void BoolEquation::Rule3ColNull(BBV vector)
{
    for (int i = 0; i < vector.getSize(); i++) {
        if (vector[i] == 1 && mask[i] != 1) {
            mask.Set1(i);  // Маскируем эту переменную
        }
    }
}

// Правило 4: все значения столбца — 0 → переменная должна быть 0
bool BoolEquation::Rule4Col0(BBV vector)
{
    for (int i = 0; i < vector.getSize(); i++) {
        if (vector[i] == 0 && mask[i] != 1) {
            Simplify(i, '0');
            return true;
        }
    }
    return false;
}

// Правило 5: все значения столбца — 1 → переменная должна быть 1
bool BoolEquation::Rule5Col1(BBV vector)
{
    for (int i = 0; i < vector.getSize(); i++) {
        if (vector[i] == 1 && mask[i] != 1) {
            Simplify(i, '1');
            return true;
        }
    }
    return false;
}

// Применяет значение к переменной в колонке ixCol и удаляет строки, которые становятся выполненными
void BoolEquation::Simplify(int ixCol, char value)
{
    for (int i = 0; i < cnfSize; i++) {
        BoolInterval *interval = cnf[i];
        if (interval != nullptr) {
            char val = interval->getValue(ixCol);
            if (val == value && val != '-') {
                cnf[i] = nullptr;
                count--; // Удаляем строку из рассмотрения
            }
        }
    }

    root->setValue(value, ixCol);  // Сохраняем решение
    mask.Set1(ixCol);              // Маскируем переменную
}

// Стратегия выбора переменной для ветвления (жадная: выбирает переменную с наименьшей определённостью)
int ColumnStrategy::chooseVarForBranching(BoolEquation* equation)
{
    vector<int> indexes; // Индексы незамаскированных переменных
    vector<int> values;  // Счётчики неопределённости

    BBV mask = equation->GetMask();
    for (int i = 0; i < mask.getSize(); i++) {
        if (mask[i] == 0) {
            indexes.push_back(i);
        }
    }

    int cnfSize = equation->GetcnfSize();
    BoolInterval** cnf = equation->GetCnf();

    bool rezInit = false;
    for (int i = 0; i < cnfSize; i++) {
        BoolInterval *interval = cnf[i];
        if (interval != nullptr) {
            if (!rezInit) {
                for (int k = 0; k < (int)indexes.size(); k++) {
                    values.push_back(interval->getValue(indexes[k]) == '-' ? 1 : 0);
                }
                rezInit = true;
            } else {
                for (int k = 0; k < (int)indexes.size(); k++) {
                    if (interval->getValue(indexes[k]) == '-') {
                        values[k]++;
                    }
                }
            }
        }
    }

    if (values.empty())
        return -1;

    // Возвращаем переменную с наименьшей частотой появления "-"
    int minElementIndex = std::min_element(values.begin(), values.end()) - values.begin();
    return indexes[minElementIndex];
}
/*
Дальнейшие шаги после boolequation.cpp
BBV.h / BBV.cpp
− Реализует побитовый вектор: хранение битов, логические операции (AND, OR, XOR, NOT), сдвиги, доступ по индексу, подсчет веса.
− Используется в BoolInterval и BoolEquation для масок и агрегирования.

NodeBoolTree.h / NodeBoolTree.cpp
− Описывает узел дерева DPLL: поля BoolEquation* eq, NodeBoolTree* lt, *rt.
− Ветвление создаёт новые узлы через placement new и добавляет их в стек в main.cpp.

Allocator.h / Allocator.cpp
− Пул блоков для ускорения new/delete объектов BoolEquation и NodeBoolTree.
− Используется в main.cpp при placement new для копий уравнения и узлов.

main.cpp
− Точка входа: чтение .pla, построение CNF, запуск цикла DPLL через стек.
− Последовательно вызывает CheckRules(), Simplify(), стратегию chooseVarForBranching, создает новые узлы и, в конце, выводит найденное решение.

*/