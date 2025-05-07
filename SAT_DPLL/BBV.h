#ifndef BBV_H
#define BBV_H

#include <iostream>
using namespace std;

typedef unsigned char byte;  // using 8-bit unsigned for bit storage

class X
{
    byte* ptr;   // указатель на байт, содержащий бит
    int index;   // индекс бита внутри байта
public:
    X();  // конструктор по умолчанию
    X(byte* vec, int k);  // инициализация с указателем и смещением
    X operator=(int k);  // присваивание бита
    operator int();      // чтение бита
    X operator=(X& v);   // копирующее присваивание
};

class BBV
{
    friend X;
    byte* vec;  // массив байт для хранения битового вектора
    int size;   // количество байт
    int len;    // общее число бит
public:
    ~BBV();
    BBV();
    BBV(int size_bits);           // инициализация по числу бит
    BBV(const char* str);         // инициализация из строки '0'/'1'
    BBV(BBV& V);                  // копирующий конструктор
    void Init(const char* str);   // перезаполнение из строки
    void Set0(int k);             // установить k-ый бит в 0
    void Set1(int k);             // установить k-ый бит в 1
    BBV operator=(BBV& V);
    BBV operator=(const char* str);
    bool operator==(BBV& V);
    BBV operator|(BBV& V);
    BBV operator&(BBV& V);
    BBV operator^(BBV& V);
    BBV operator~();
    BBV operator>>(int k);
    BBV operator<<(int k);
    X operator[](int k);
    operator char*();
    int getWeight();
    int getSize();
    friend ostream& operator<<(ostream& r, BBV& V);
    friend istream& operator>>(istream& r, BBV& V);
};

#endif // BBV_H
