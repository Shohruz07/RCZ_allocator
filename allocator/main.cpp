#include <iostream>
#include "Allocator.h"
#include "DataTypes.h"
#include <new>       // std::set_new_handler std::bad_alloc (set_new_handler - устанавливает функцию обработчик для исключения new)
#include <ctime>



void my_out_of_memory() {
    std::cerr << "Ошибка: недостаточно памяти для выделения." << std::endl;
    throw std::bad_alloc();  // bad_alloc - исключение вызывается при неудачном выделении памяти
}

class MyClass
{
private:
    DECLARE_ALLOCATOR
    int* arr = nullptr;
    size_t size = 0;

public:


    MyClass(const size_t size_)
    {
        std::cout << "Calling the constructor" << std::endl;
        size = size_;
        arr = (int*)operator new[](size * sizeof(int));
    }

    int* GetIndex(const size_t idx)
    {
        return arr + idx;
    }


    ~MyClass()
    {
        std::cout << "Calling the destructor " << std::endl;
        operator delete[](arr);
    }

};


IMPLEMENT_ALLOCATOR(MyClass, 0, 0)



int main()
{
   

    //функция обработчик функцию, будет вызываться, если оператор new не сможет 
    // выделать память, функция обработчик для ошибки new set_new_handler устанавливает функицю
    // которая будет вызываться при ошибке my_out_of_memory
    std::set_new_handler(my_out_of_memory);

    setlocale(LC_ALL, "ru");
    srand(time(NULL));


    try {

        //тестирование отлавливания исключений, вместо этого тестирования можно сделать ошибку 
        //раскоментировав строчку 176 или 218 и удалить код с 68 по 81 строку чтобы без палева

        //создаю аллокатор на 40 байт 
        //Allocator toBreak(40);
        ////выделяю с аллокатора себе память под класс
        //void* obj = toBreak.Allocate(sizeof(MyClass));

        //тут специально большой размер который new не может выделить чтобы new выбросил исключение
        //использую механизм размещения объекта в уже выделенной памяти, при этом ответственость за вызов деструтора 
        //кладется на программиста
        
        //происходит вызов дестркутора, но оператор new не может создать такой большой объект, поэтому он выбрасывает искулючение
/*        MyClass* objFromObj = ::new (obj) MyClass(1000000000000000000); 
        objFromObj->~MyClass();
        toBreak.Deallocate(obj)*/;


        //режим блоков кучи. создается куча размером 40 байт, можем брать из неё 20 байт, тогда сможет в последующем взять 
        //ещё 20. если захотели взять все 40, то все равно сможем создать из этой кучи ещё 40 (Но не больше 40-ка)
        //этот режим позволяет создавать столько объектов размером 40 (или столько сколько байт передать) но привысить размер
        //байт при попытке выделить память приведет к ошибке. 
        //то есть можем создавать хоть 1000 объектов по 40 байт но не больше.
        //при всем этом, когда выделяется память то она берется из кучи, но при вызове фунции deallocate она не возвращается
        //в кучу, она хнарится в списке и при новом создании объекта берется свободный блок из этого списка
        //как я понимаю, при этом, пытаться выделять ещё объектов и все элементы списка будут использоваться, то выделится из
        //кучи новая память и она станет частью списка и не вернется обратно в кучу
        //в каждом из режимов - этот список нужен чтобы решить проблему фрагментации памяти и увеличить скорость работы. при этом,
        //если в списке не осталось свободной памяти, то будет выделтся ещё из кучи, это значит что данный режим медленнее всех,
        //т.к. тут выделяются новые блоки, нет фиксированного количества

        Allocator allocatorHeapBloks(50); //в будущем хочу использовать память для создания массива из 10 элементов типа int


        int* numbers = (int*)allocatorHeapBloks.Allocate(10 * sizeof(int)); //если будет 11 то будет ошибка
        if (numbers)
        {
            for (int i = 0; i < 10; i++)
            {
                numbers[i] = i + 1;
                std::cout << "HeapBloks_numbers[ " << i << " ] = " << numbers[i] << std::endl;
            }
        }
        else
        {
            std::cerr << "Memory allocation error";
        }

        //в случае с режимом блоков кучи с неограниченными блоками можем много раз выделять блоки такого размены
        //который указан в алокаторе
        int* numbersTry = (int*)allocatorHeapBloks.Allocate(10 * sizeof(int)); //если будет 11 то будет ошибка
        if (numbersTry)
        {
            for (int i = 0; i < 10; i++)
            {
                numbersTry[i] = i + 1;
                std::cout << "HeapBloks_numbersTry[ " << i << " ] = " << numbersTry[i] << std::endl;
            }
        }
        else
        {
            std::cerr << "Memory allocation error";
        }


        allocatorHeapBloks.Deallocate(numbers);

        std::cout << std::endl;


        //в случае с режимом пула кучи мы выделяем кол-во блоков (2 сейчас) с нужным количеством байт (20 сейчас)
        //в этом режиме создается список из 2 элементов и в отличие от обычного режима кучи, невозможно будет дополнить список
        //чуть чуть быстрее прошлого режима, т.к. тут не создаются новые блоки а в самом начале выделяется память под 2 блока
        // 40 байт, или столько сколько надо, например можно 10000 байт 50 блоков и больше, главное чтобы у компа хватило ресурсов

        Allocator allocatorHeapPool(50, 2);
        int* numbers1 = (int*)allocatorHeapPool.Allocate(10 * sizeof(int)); //если будет 11 то будет ошибка
        if (numbers1)
        {
            for (int i = 0; i < 10; i++)
            {
                numbers1[i] = i + 1;
                std::cout << "HeapPool_numbers1[ " << i << " ] = " << numbers1[i] << std::endl;
            }
        }
        else
        {
            std::cerr << "Memory allocation error";
        }

        std::cout << std::endl;

        //если будет 11 то будет ошибка + если используем больше блоков чем разрешено (сейчас 2) то будет ошибка
        int* numbers2 = (int*)allocatorHeapPool.Allocate(8 * sizeof(int));
        if (numbers2)
        {
            for (int i = 0; i < 8; i++)
            {
                numbers2[i] = i + 1;
                std::cout << "HeapPool_numbers2[ " << i << " ] = " << numbers2[i] << std::endl;
            }
        }
        else
        {
            std::cerr << "Memory allocation error";
        }

        //если попытаться использовать 3 блок памяти из пула которого нет то будет ошибка (которая ловится блоком catch)
        //, ведь мы задейстовли все блоки пусть и не полностью
        //не смотря на то что полседний блок был задейстован не полность, а всего 8 элементов типа int

        //int* numbers3 = (int*)allocatorHeapPool.Allocate(10 * sizeof(int));
        //if (numbers3)
        //{
        //    for (int i = 0; i < 10; i++)
        //    {
        //        numbers3[i] = i + 1;
        //        std::cout << "numbers3[ " << i << " ] = " << numbers3[i] << std::endl;
        //    }
        //}
        //else
        //{
        //    std::cerr << "Ошибка выделения памяти";
        //}

        // Режим статического пула с 2, 50 байтовыми блоками
        // быстрее всех, т.к. память выделяется не из кучи а из статической памяти, тоже имеет фиксированное количество блоков

        char staticMemoryPool[50 * 2];
        Allocator allocatorStaticPool(50, 2, staticMemoryPool);

        //тут я получвется создал аллокатор из 80 байт, из типа char
        //но выделяю память для int, ничего страшного, ведь я выделаю не больше 40 байт для int и явно преобразую в int* 
        int* numbersFromStatic = (int*)allocatorStaticPool.Allocate(10 * sizeof(int));
        if (numbersFromStatic)
        {
            for (int i = 0; i < 10; i++)
            {
                numbersFromStatic[i] = i + 1;
                std::cout << "numbersFromStatic[ " << i << " ] = " << numbersFromStatic[i] << std::endl;
            }
        }
        else
        {
            std::cerr << "Memory allocation error";
        }

        //после этого по идее, использовал 1 блок размером 40 байт
        //пытаюсь использовать 2 блок и беру из него 10 байт всего для массива char
        char* charactersFromStatic = (char*)allocatorStaticPool.Allocate(10 * sizeof(char));

        //вроде бы осталось ещё 30 байт для того что хочешь, но, т.к. этот блок задейстован, то
        //при попытке использовать эти 30 байт будет ошибка (она кстати отлавливается блоком catch)
        //char* charactersFromSratic1 = (char*)allocatorStaticPool.Allocate(1 * sizeof(char));



        size_t sizeOfArrInClass = 100000;
        MyClass* obj1 = new MyClass(sizeOfArrInClass);

        std::cout << "Enter " << sizeOfArrInClass << " array elements" << std::endl;
        for (int i = 0, number = 0; i < sizeOfArrInClass; i++)
        {
            //std::cin >> number;
            *(obj1->GetIndex(i)) = 0 + rand() % 11;
        }

        for (int i = 0; i < sizeOfArrInClass; i++)
        {
            //std::cout << "ArrInClass[" << i << "] = " << *(obj1->GetIndex(i)) << std::endl;
        }

        delete obj1;
    }
    catch (const std::bad_alloc& e) {
        std::cerr << "Exception : " << e.what() << std::endl;
        return 333;
    }
     std::cout << "Press the enter key to terminate the program";
     std::cin.get();
    return 0;
}