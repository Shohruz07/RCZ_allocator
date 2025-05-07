#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stack>
#include <chrono>
#include <stdexcept>
#include <cctype>

// Подключаем все основные модули, участвующие в решении SAT через DPLL
#include "Allocator.h"         // Кастомный аллокатор памяти (memory pool)
#include "NodeBoolTree.h"      // Узлы дерева поиска решений
#include "boolinterval.h"      // Представление одной дизъюнкции в CNF (булев интервал)
#include "boolequation.h"      // Основная логика уравнения и применения DPLL
#include "BBV.h"               // Представление булевых векторов с "don't care"

using namespace std;

// Удаляет пробелы и символы переноса с начала и конца строки
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

int main(int argc, char *argv[])
{
    auto startTimeSuperGeneral = chrono::high_resolution_clock::now(); // Засекаем общее время работы

    vector<string> full_file_list; // Хранит строки из .pla файла (каждая строка — дизъюнкт)
    string filepath = "/home/kali/Desktop/Lab2WithOutQtAndWithAllocator/SAT_DPLL/SatExamples/Sat_ex14_3.pla";
    ifstream file(filepath);
    
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            // Удаляем возможный символ '\r' в конце строки 
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            full_file_list.push_back(trim(line)); // Добавляем очищенную строку
        }
        file.close();
        
        int cnfSize = full_file_list.size(); // Число дизъюнктов (строк)
        BoolInterval **CNF = new BoolInterval*[cnfSize]; // Массив указателей на дизъюнкты
        int rangInterval = -1;

        // Определяем количество переменных по длине первой строки
        if (cnfSize > 0) {
            rangInterval = trim(full_file_list[0]).length();
        }

        // Создаем каждый дизъюнкт из строки
        for (int i = 0; i < cnfSize; i++) {
            string trimmed_line = trim(full_file_list[i]);
            CNF[i] = new BoolInterval(trimmed_line.c_str());
        }

        // Формируем корень дерева поиска:
        // вектор значений: все 0, вектор маски: все 1 ("don't care")
        string rootvec, rootdnc;
        for (int i = 0; i < rangInterval; i++) {
            rootvec.push_back('0');
            rootdnc.push_back('1');
        }
        BBV vec(rootvec.c_str());   // Вектор значений переменных
        BBV dnc(rootdnc.c_str());   // Маска "don't care"
        
        BoolInterval *root = new BoolInterval(vec, dnc); // Интервал, покрывающий все возможные варианты
        BoolEquation *boolequation = new BoolEquation(CNF, root, cnfSize, cnfSize, vec); // Инициализация булевого уравнения
        
        boolequation->SetStrategy(new ColumnStrategy()); // Устанавливаем стратегию ветвления (по колонке)

        bool rootIsFinded = false; // Флаг, найдено ли решение
        stack<NodeBoolTree*> BoolTree; // Стек дерева поиска
        NodeBoolTree *startNode = new NodeBoolTree(boolequation); // Начальный узел дерева
        BoolTree.push(startNode); // Помещаем его в стек
        
        // Создаем аллокаторы для двух типов объектов
        Allocator allocatorBoolEq(sizeof(BoolEquation), 10000); // Аллокатор для BoolEquation
        Allocator allocatorNode(sizeof(NodeBoolTree), 10000);   // Аллокатор для NodeBoolTree

        // Основной цикл DPLL
        do {
            NodeBoolTree *currentNode = BoolTree.top(); // Берем текущий узел

            // Если у него нет потомков — развиваем его
            if (currentNode->lt == nullptr && currentNode->rt == nullptr) {
                BoolEquation *currentEquation = currentNode->eq; // Получаем уравнение
                
                // Назначаем стратегию, если ещё не назначена
                if (currentEquation->GetStrategy() == nullptr) {
                    currentEquation->SetStrategy(new ColumnStrategy());
                }

                bool flag = true;
                while (flag) {
                    int a = currentEquation->CheckRules(); // Применяем правила упрощения

                    switch (a) {
                        case 0: {
                            // Формула противоречива — отбрасываем этот путь
                            BoolTree.pop(); 
                            flag = false;
                            break;
                        }
                        case 1: {
                            // Формула решена или остались только "don't care"
                            if (currentEquation->GetCount() == 0 ||
                                currentEquation->GetMask().getWeight() == currentEquation->GetMask().getSize()) {
                                flag = false;
                                rootIsFinded = true;

                                // Проверяем, покрывает ли решение все исходные дизъюнкты
                                for (int i = 0; i < cnfSize; i++) {
                                    if (!CNF[i]->isEqualComponent(*currentEquation->GetRoot())) {
                                        rootIsFinded = false;
                                        BoolTree.pop();
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        case 2: {
                            // Нужно сделать ветвление по переменной
                            int indexBranching = currentEquation->GetStrategy()->chooseVarForBranching(currentEquation);
                            if (indexBranching < 0) {
                                flag = false;
                                break;
                            }

                            // Клонируем уравнение дважды: под 0 и под 1, используя кастомный аллокатор (placement new)
                            BoolEquation* Equation0Alloc = new (allocatorBoolEq.Allocate(sizeof(BoolEquation))) BoolEquation(*currentEquation);
                            BoolEquation* Equation1Alloc = new (allocatorBoolEq.Allocate(sizeof(BoolEquation))) BoolEquation(*currentEquation);

                            // Упрощаем каждое уравнение по выбранной переменной
                            Equation0Alloc->Simplify(indexBranching, '0');
                            Equation1Alloc->Simplify(indexBranching, '1');

                            // Создаем новые узлы дерева
                            NodeBoolTree *Node0_alloc = new (allocatorNode.Allocate(sizeof(NodeBoolTree))) NodeBoolTree(Equation0Alloc);
                            NodeBoolTree *Node1_alloc = new (allocatorNode.Allocate(sizeof(NodeBoolTree))) NodeBoolTree(Equation1Alloc);

                             // 3. Привязываем созданные узлы к текущему
                            currentNode->lt = Node0_alloc; // левая ветвь = 0
                            currentNode->rt = Node1_alloc; // правая ветвь = 1

                            // Добавляем в стек для дальнейшей обработки
                            BoolTree.push(Node0_alloc);
                            BoolTree.push(Node1_alloc);

                            flag = false;
                            break;
                        }
                    }
                }
            } else {
                // Если потомки есть — узел уже обработан
                BoolTree.pop();
            }
        } while (BoolTree.size() > 1 && !rootIsFinded); // Пока дерево не "схлопнется" или не найдём решение

        // Вывод результата
        if (rootIsFinded) {
            cout << "Root is:\n ";
            BoolInterval *finded_root = BoolTree.top()->eq->GetRoot(); // Получаем найденный интервал
            cout << string(*finded_root); // Печатаем его
        } else {
            cout << "Root does not exist!"; // Решение отсутствует
        }
    } else {
        cout << "File does not exist.\n"; // Файл не открыт
    }

    // Замер времени работы
    auto endTimeSuperGeneral = chrono::high_resolution_clock::now();
    chrono::duration<double> durationSuperGeneral = endTimeSuperGeneral - startTimeSuperGeneral;
    cout << "\n Время работы программы с allocator: " 
         << chrono::duration_cast<chrono::microseconds>(durationSuperGeneral).count() 
         << " микросек.\n";

    return 0;
}
