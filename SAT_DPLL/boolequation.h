#ifndef BOOLEQUATION_H
#define BOOLEQUATION_H

#include "boolinterval.h"

// Вперёд объявляем интерфейс стратегии ветвления
class IBranchStrategy;

/*
 * Класс BoolEquation представляет булеву формулу в виде КНФ,
 * где каждая дизъюнкция (строка) кодируется с помощью BoolInterval.
 */
class BoolEquation
{
private:
	IBranchStrategy* strategy = nullptr; // Стратегия выбора переменной для ветвления
	BoolInterval **cnf;    // Указатели на дизъюнкции в КНФ (строки)
	BoolInterval *root;    // Корень — итоговое решение (присваивание переменных)
	int cnfSize;           // Размер массива cnf (включая nullptr строки)
	int count;             // Количество активных строк (ненулевых дизъюнкций)
	BBV mask;              // Маска — определяет, какие переменные уже определены (1 — использовать нельзя)
public:
	// Конструктор с параметрами
	BoolEquation(BoolInterval **cnf, BoolInterval *root, int cnfSize, int count, BBV mask);

	// Конструктор копирования
	BoolEquation(BoolEquation &equation);

	// Проверка правила 2: строка пустая (все don't care и известные переменные)
	bool Rule2RowNull(BoolInterval *interval);

	// Проверка правила 1: в строке ровно одна незамаскированная переменная
	bool Rule1Row1(BoolInterval *interval);

	// Упрощение формулы: применяет присваивание переменной и удаляет выполненные строки
	void Simplify(int ixCol, char value);

	// Применяет правило 3: удаляет переменные, значения которых не влияют ни на одну строку
	void Rule3ColNull(BBV vector);

	// Выбор переменной для ветвления (если не используется стратегия)
	int ChooseColForBranching();

	// Проверка правила 5: столбец состоит из одних единиц → переменная должна быть 1
	bool Rule5Col1(BBV vector);

	// Проверка правила 4: столбец состоит из одних нулей → переменная должна быть 0
	bool Rule4Col0(BBV vector);

	// Проверка применимости всех правил упрощения (возвращает 0 — UNSAT, 1 — упрощено, 2 — ничего не сделано)
	int CheckRules();

	// Геттеры:
    IBranchStrategy* GetStrategy() { return strategy; }
	BoolInterval*    GetRoot()     { return root; }
	BoolInterval**   GetCnf()      { return cnf; }
	int              GetcnfSize()  { return cnfSize; }
	int              GetCount()    { return count; }
	BBV              GetMask()     { return mask; }

	// Сеттер для стратегии ветвления
	void SetStrategy(IBranchStrategy* strategy_) { strategy = strategy_; }
};

/*
 * Интерфейс для стратегий ветвления (выбора переменной на шаге ветвления).
 * Позволяет легко подставлять разные подходы к выбору переменных.
 */
class IBranchStrategy {
public:
	virtual ~IBranchStrategy() = default;

	// Метод выбирает переменную для ветвления и возвращает её индекс.
	// Если возвращает -1 — возможно, стратегия не выбрала переменную (например, выбрала строку).
	virtual int chooseVarForBranching(BoolEquation* equation) = 0;
};

/*
 * Конкретная реализация стратегии ветвления.
 * ColumnStrategy выбирает переменную (столбец) на основе жадного анализа количества неопределённостей.
 */
class ColumnStrategy : public IBranchStrategy {
public:
	int chooseVarForBranching(BoolEquation* equation) override;
};

#endif // BOOLEQUATION_H
