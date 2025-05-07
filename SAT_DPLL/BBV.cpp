#include "BBV.h"
#include <iostream>
#include <cstring>

// BBV.cpp: Реализация битового вектора и доступа к отдельным битам через вспомогательный класс X.

// Деструктор: освобождаем выделенный массив байт, если он существует.
BBV::~BBV()
{
    if (vec != nullptr)
        delete[] vec;
}

// Конструктор по умолчанию: пустой вектор
BBV::BBV()
    : vec(nullptr), size(0), len(0)
{}

// Основной конструктор: выделение под len бит
BBV::BBV(int size_v)
{
    if (size_v <= 0)
        throw 0;              // некорректный размер
    len  = size_v;
    size = (len - 1) / 8 + 1; // рассчитываем число байт
    vec  = new byte[size];    // выделяем память под байты
    if (!vec) throw 0;
    // Обнуляем все биты
    for (int i = 0; i < size; i++)
        vec[i] = 0;
}

// Инициализация из строки '0101...'
BBV::BBV(const char *str)
{
    if (!str) throw 0;
    len  = strlen(str);
    size = (len - 1) / 8 + 1;
    vec  = new byte[size];
    if (!vec) throw 0;

    int bitCount = 8, byteIndex = 0;
    byte mask = (byte)1;
    // Чистим первый байт
    vec[0] = 0;
    for (int i = 0; i < len; /* инкремент внутри */) {
        if (bitCount > 0) {
            // выставляем бит, если символ != '0'
            if (str[i] != '0')
                vec[byteIndex] |= mask;
            mask <<= 1;
            bitCount--;
            i++;
        } else {
            // переходим к следующему байту
            bitCount = 8;
            byteIndex++;
            vec[byteIndex] = 0;
            mask = (byte)1;
        }
    }
}

// Копирующий конструктор: копируем len, size и содержимое массива
BBV::BBV(BBV &V)
{
    if (!V.vec) throw 0;
    len  = V.len;
    size = V.size;
    vec  = new byte[size];
    if (!vec) throw 0;
    for (int i = 0; i < size; i++)
        vec[i] = V.vec[i];
}

// Переинициализация из строки той же длины
void BBV::Init(const char *str)
{
    if (!str) throw 1;
    len  = strlen(str);
    size = (len - 1) / 8 + 1;
    if (vec) delete[] vec;
    vec = new byte[size];
    if (!vec) throw 0;
    int bitCount = 8, byteIndex = 0;
    byte mask = (byte)1;
    vec[0] = 0;
    for (int i = 0; i < len;) {
        if (bitCount > 0) {
            if (str[i] != '0') vec[byteIndex] |= mask;
            mask <<= 1;
            bitCount--; i++;
        } else {
            bitCount = 8; byteIndex++; vec[byteIndex] = 0; mask = (byte)1;
        }
    }
}

// Сброс бита k в 0 и 1
void BBV::Set0(int k)
{
    if (!vec || k < 0 || k >= len) throw 1;
    int byteIndex = k / 8;
    int bitIndex  = k % 8;
    vec[byteIndex] &= ~((byte)1 << bitIndex);
}

void BBV::Set1(int k)
{
    if (!vec || k < 0 || k >= len) throw 1;
    int byteIndex = k / 8;
    int bitIndex  = k % 8;
    vec[byteIndex] |= ((byte)1 << bitIndex);
}

// Операторы присваивания и логических операций
BBV BBV::operator=(BBV &V)
{
    if (this != &V) {
        len = V.len;
        size = V.size;
        if (vec) delete[] vec;
        vec = new byte[size];
        if (!vec) throw 0;
        for (int i = 0; i < size; i++) vec[i] = V.vec[i];
    }
    return *this;
}

BBV BBV::operator=(const char *str)
{
    Init(str);
    return *this;
}

bool BBV::operator==(BBV &V)
{
    if (!vec || !V.vec || V.len != len) return false;
    for (int i = 0; i < size; i++)
        if (vec[i] != V.vec[i]) return false;
    return true;
}

BBV BBV::operator|(BBV &V)
{
    if (!vec || !V.vec || V.len != len) throw 2;
    BBV res(*this);
    for (int i = 0; i < size; i++) res.vec[i] |= V.vec[i];
    return res;
}

BBV BBV::operator&(BBV &V)
{
    if (!vec || !V.vec || V.len != len) throw 2;
    BBV res(*this);
    for (int i = 0; i < size; i++) res.vec[i] &= V.vec[i];
    return res;
}

BBV BBV::operator^(BBV &V)
{
    if (!vec || !V.vec || V.len != len) throw 2;
    BBV res(*this);
    for (int i = 0; i < size; i++) res.vec[i] ^= V.vec[i];
    return res;
}

BBV BBV::operator~()
{
    BBV res(*this);
    for (int i = 0; i < size; i++) res.vec[i] = ~res.vec[i];
    int tail = len % 8;
    if (tail) {
        res.vec[size-1] &= ((1 << tail) - 1);
    }
    return res;
}

// Сдвиги
BBV BBV::operator>>(int k)
{
    BBV res(*this);
    if (vec && k > 0) {
        for (int i = 0; i < size; i++) res.vec[i] = 0;
        int bs = k / 8, b = k % 8;
        for (int i = 0; i < size; i++) {
            int src = i + bs;
            if (src < size) {
                res.vec[i] |= vec[src] >> b;
                if (b && src+1 < size)
                    res.vec[i] |= vec[src+1] << (8-b);
            }
        }
    }
    return res;
}

BBV BBV::operator<<(int k)
{
    BBV res(*this);
    if (vec && k > 0) {
        for (int i = 0; i < size; i++) res.vec[i] = 0;
        int bs = k/8, b = k%8;
        for (int i = size-1; i >= 0; i--) {
            int src = i - bs;
            if (src >= 0) {
                res.vec[i] |= vec[src] << b;
                if (b && src-1 >= 0)
                    res.vec[i] |= vec[src-1] >> (8-b);
            }
        }
    }
    return res;
}

// Доступ к биту
X BBV::operator[](int k)
{
    if (!vec || k<0 || k>=len) throw 1;
    return X(&vec[k/8], k%8);
}

// Преобразование в строку '0101...'
BBV::operator char*()
{
    if (!vec) return nullptr;
    char* out = new char[len+1];
    int bi = 0; byte m = (byte)1;
    for(int i=0;i<len;i++){
        if(i>0 && i%8==0){ bi++; m=(byte)1; }
        out[i] = (vec[bi]&m)?'1':'0'; m<<=1;
    }
    out[len] = '\0';
    return out;
}

int BBV::getWeight()
{
    int cnt=0;
    for(int i=0;i<size;i++){
        byte b = vec[i];
        while(b){ cnt++; b &= b- (byte)1; }
    }
    return cnt;
}

int BBV::getSize(){ return len; }

// Класс X — обёртка для доступа к одному биту
X::X(): ptr(nullptr), index(0) {}
X::X(byte* v,int k): ptr(v), index(k) {}

X X::operator=(int v){
    byte m=(byte)1<<index; if(v) *ptr|=m; else *ptr&=~m; return *this;
}
X::operator int(){ return ((*ptr & ((byte)1<<index))!= (byte)0)?1:0; }
X X::operator=(X& v){ return (*this = int(v)); }

// Вывод и ввод для отладки
ostream& operator<<(ostream&r,BBV&V){
    for(int i=0, bi=0;i<V.len;i++){
        if(i>0&&i%8==0){ bi++; }
        byte m=(byte)1<<(i%8);
        r << ((V.vec[bi]&m)?'1':'0');
    }
    return r;
}
istream& operator>>(istream&r,BBV&V){
    // далее идёт ввод через консоль — редко используется в DPLL
    return r;
}

/*
Дальнейшие шаги проекта:
1) boolequation.cpp/.h         — реализация правил DPLL (unit-propagation, pure-literal, branching)
2) boolinterval.cpp/.h        — хранение одной клаузы и её упрощение
3) NodeBoolTree.h             — структура узла дерева поиска
4) main.cpp                   — объединяет всё: читает .pla, строит дерево, запускает DPLL
*/
