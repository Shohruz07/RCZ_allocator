#include "boolinterval.h"
#include "BBV.h"
#include <cstring>
#include <iostream>

// Класс BoolInterval представляет один дизъюнкт (клаузу) CNF,
// где каждая переменная может принимать значения:
// '0' (false), '1' (true) или '-' (don't care — не важно).

// ░░ Конструктор по длине: создаёт пустой интервал длины len,
//    изначально все биты = 0, все флаги don’t care = 0.
BoolInterval::BoolInterval(size_t len)
{
    vec = BBV(len);  // создаём BBV-вектор для значений
    dnc = BBV(len);  // создаём BBV-вектор для don’t care
}

// ░░ Конструктор из двух строк: vec_in содержит '0'/'1',
//    dnc_in содержит '0'/'1' где 1 = don’t care.
//    Если строки некорректные или разной длины — создаём дефолтные длиной 8.
BoolInterval::BoolInterval(const char *vec_in, const char *dnc_in)
{
    if (vec_in && dnc_in && strlen(vec_in) == strlen(dnc_in)) {
        vec = BBV(vec_in);
        dnc = BBV(dnc_in);
    } else {
        // Защита от неверных входных данных
        vec = BBV(8);
        dnc = BBV(8);
    }
}

// ░░ Конструктор из одной строки PLA-формата, например "1-0-":
//    '-' → don’t care, '1' → vec=1, остальные (включая '0') → vec=0.
BoolInterval::BoolInterval(const char *vector)
{
    if (vector) {
        size_t sz = strlen(vector);
        vec = BBV(sz);
        dnc = BBV(sz);
        for (size_t ix = 0; ix < sz; ix++) {
            if (vector[ix] == '-') {
                dnc[ix] = 1;       // помечаем don’t care
            } else if (vector[ix] == '1') {
                vec[ix] = 1;       // ставим бит 1
            }
            // '0' или любой другой → оставляем 0
        }
    }
}

// ░░ Конструктор из готовых BBV-векторов (deep copy по значению)
BoolInterval::BoolInterval(BBV &vec_in, BBV &dnc_in)
    : vec(vec_in), dnc(dnc_in)
{}

// ░░ Здесь можно реализовать массовую установку интервала,
//    но метод оставлен пустым.
void BoolInterval::setInterval(BBV &vec, BBV &dnc)
{
    // this->vec = vec;
    // this->dnc = dnc;
}

// ░░ Оператор ==: возвращает true, если оба BBV-вектора совпадают.
bool BoolInterval::operator==(BoolInterval &ibv)
{
    return (vec == ibv.vec && dnc == ibv.dnc);
}

// ░░ Оператор !=: возвращает true, если хотя бы один BBV-вектор отличается.
bool BoolInterval::operator!=(BoolInterval &ibv)
{
    return (vec != ibv.vec || dnc != ibv.dnc);
}

// ░░ Преобразование в std::string вида "1-01" по getValue().
BoolInterval::operator string()
{
    size_t sz = vec.getSize();
    string str(sz, '0');
    for (size_t ix = 0; ix < sz; ix++) {
        str[ix] = getValue(ix);
    }
    return str;
}

// ░░ Возвращает длину интервала (количество переменных).
int BoolInterval::length()
{
    return vec.getSize();
}

// ░░ Ранг = количество фиксированных битов = total - don’t care.
int BoolInterval::rang()
{
    return vec.getSize() - dnc.getWeight();
}

// ░░ Проверка совместимости (используется для финального сравнения найденного root).
//    true, если интервалы можно считать одинаковыми по смыслу.
bool BoolInterval::isEqualComponent(BoolInterval &ibv)
{
    // zero — вектор нулей того же размера
    BBV zero(vec.getSize()), tmpUV(zero), tmpU(zero), tmpV(zero), answer(zero);

    tmpUV = dnc | ibv.dnc;               // объединённая маска don’t care
    tmpU  = vec | tmpUV;                // учитываем don’t care
    tmpV  = ibv.vec | tmpUV;
    answer = (tmpU ^ tmpV) | tmpUV;     // XOR и don’t care
    // если после учёта don’t care остались различия во всех позициях,
    // значит интервалы эквивалентны
    return answer.getWeight() != vec.getSize();
}

// ░░ Проверка ортогональности: true, если есть конфликт значимых битов
bool BoolInterval::isOrthogonal(BoolInterval &ibv)
{
    BBV zero(vec.getSize()), tmpUV(zero), tmpU(zero), tmpV(zero), answer(zero);

    tmpUV = dnc | ibv.dnc;
    tmpU  = vec    | tmpUV;
    tmpV  = ibv.vec| tmpUV;
    answer = tmpU ^ tmpV;               // позиционные конфликты
    return (answer != zero);
}

// ░░ Возвращает символ в позиции ix: '-', '1' или '0'.
char BoolInterval::getValue(int ix)
{
    if (ix < 0 || ix >= vec.getSize())
        throw "Out of range";
    if (dnc[ix] == 1) return '-';
    if (vec[ix] == 1) return '1';
    return '0';
}

// ░░ Устанавливает значение в позиции ix: '-', '0' или '1'.
void BoolInterval::setValue(char value, int ix)
{
    if (ix < 0 || ix >= vec.getSize())
        throw "Out of range";

    if (value == '-') {
        dnc[ix] = 1; vec[ix] = 0;   // don't care
    } else if (value == '0') {
        vec[ix] = 0; dnc[ix] = 0;   // фиксируем 0
    } else {  // '1'
        vec[ix] = 1; dnc[ix] = 0;   // фиксируем 1
    }
}


/*
 Дальнейшие шаги после BoolInterval
BBV.h / BBV.cpp

Определение побитового вектора BBV, методы логических операций, сдвигов, доступа к битам (operator[]), подсчёт веса (getWeight()).

boolequation.h / boolequation.cpp

Реализация логики DPLL: методы CheckRules(), Simplify(), правила Rule1–Rule5, стратегия ветвления ColumnStrategy.

NodeBoolTree.h

Структура узла дерева поиска DPLL с полями eq, lt, rt.

Allocator.h / Allocator.cpp

Пул аллокации объектов BoolEquation и NodeBoolTree для ускоренного placement new.

main.cpp

Точка входа: чтение .pla, построение BoolInterval[], создание BoolEquation, запуск DPLL-цикла с явным стеком и placement new для ветвления.

Таким образом весь поток:

ruby
Копировать
Редактировать
main.cpp
  └→ BoolInterval (парсинг строк)
        └→ BBV (низкоуровневый битовый вектор)
  └→ BoolEquation::CheckRules/Simplify (логи DPLL)
        └→ ColumnStrategy (ветвление)
  └→ NodeBoolTree (узлы дерева)
  └→ Allocator (пул объектов)
*/