#pragma once

#include <cstdlib>
#include <stdexcept>
#include <vector> // До последнего не хотел его использовать
#include <memory>
#include "utility_units.hpp"

#define raddr std::size_t
/*
 Набор инструкций

 - Работа с памятью
 lodi    accumulator    addres          0       : запись в регистр значения из ОЗУ по прямому адресу
 lodr    accumulator    adr_reg         0       : запись в регистр значения из ОЗУ по адресу из регистра
 stri    addres         reg             0       : запись в ОЗУ значения из регистра по статичному адресу
 strr    adr_reg        reg             0       : запись в ОЗУ значения по адресу из регистра
 mov     accumulator    reg             0       : копирование значения регистра из одного в другой

 amin    min_adr        max_adr         0       : устанавливает границы доступа к памяти
 setl    0              0               0       : включает ограничение
 setf    0              0               0       : выключает его

 - Арифметика
 add     accumulator    reg1            reg2    : сложение, сумма сохраняется в аккумулятор
 addc    accumulator    reg1            value   : сложение с константой, сумма сохраняется в аккумулятор
 loc     accumulator    value           0       : запись в регистр константы
 sub     accumulator    reg1            reg2    : вычитаение, разность ложит в аккумулятор
 mult    accumulator    reg1            reg2    : умножение, ложит результат в аккумулятор
 div     accumulator    reg1            reg2    : деление, ложит частное в аккумулятор
 mod     accumulator    reg1            reg2    : деление, ложит остаток в аккумулятор

 - Сравнения и условные переходы
 cmp     reg1           reg2            0       : сравнение, меняет флаг
 jmp     condition      gotoadr         0       : условный переход
 goto    gotoadr        0               0       : безусловный переход

 - Логические операции
 or     accumulator     reg1            reg2    : логическое ИЛИ
 and    accumulator     reg1            reg2    : логическое И
 not    accumulator     reg1            0       : логическое НЕ

 - Операции работы с портами
 prts   reg             port            0       : отправка значения в порт по шине данных
 ptcs   reg             port            0       : отправить в порт управляющий сигнал из регистра
 prtg   reg             port            0       : получение значения в регистр с порта
 prts   reg             port            0       : получить с устройства сигнал состояния

 !- Операции работы со стеком
 !push   reg             0               0       : добавить в стек значение
 !pop    reg             offset          0       : получить со стека значение
 !call   reg             0               0       : вызвать функцию, автоматически сверху в стек добавляет адресс интрукции
 !ret    0               0               0       : выход из функции, переход обратно, адресс берётся с вершины стека


 !- Работа с очередью инструкций
 !intr   0               0               0       : вызов прерывания, добавляет в стек адресс инструкции
 !scall  0               0               0       : вызов системного вызова, срабатывает автоматически при достижения таймера, так же ложит в стек номер инструкции

 !- Обработка системных ошибок
 !serr   type            0               0       : установить флаг ошибки на какое-то значение
 !cerr   0               0               0       : очистить флаг ошибки
*/



namespace cpu_unit {
    inline bool check_reg_addr(std::size_t regaddr) {
        if (regaddr > 15 or regaddr < 0) return true;
        return false;
    }

    // Класс памятиs
    class memory {
    private:
        // Максимальный адрес
        std::size_t size_ram;
        // Массив ячеек
        int *m;
    public:
        // Конструктор, принимает размер памяти
        memory() : size_ram(4096) {
            m = new int[size_ram];
        }

        // Геттер из ячейки по адресу
        // adr - адрес ячейки
        // Бросается исключение при неверном адресе
        int get_from_memory(std::size_t adr) {
            if (adr < size_ram) {
                return m[adr];
            }
            else {
                throw std::runtime_error("Attempt to access non-existent memory");
            }
        }


        // Сеттер из ячейки по адресу
        // adr - адрес ячейки
        // value - значение
        // Бросается исключение при неверном адресе
        void set_to_memory(std::size_t adr, int value) {
            if (adr < size_ram) {
                m[adr] = value;
            }
            else {
                throw std::runtime_error("Attempt to access non-existent memory");
            }
        }

        // Деструктор
        ~memory() {
            delete[] m;
        }


    };


    class core {
        private:
            // Регистры, их 16 штук
            // 15 - адрес стека
            // 14 - адрес инструкции
            // 13 - адрес системных вызовов
            int registers[16];

            // Флаг сравнения
            int cmp_flag = 0;

            // Текущие декодированные значения
            int decoded[4];

            // Ограничение доступа к памяти
            int memory_addres_min;
            int memory_addres_max;
            // Если true, то процессор смотрит, есть ли доступ к адресу перед get_from_memory()
            bool safe_address_mode = false;

            // Включены ли системные вызовы, при false просто игнорирует системные вызовы
            bool syscals = false;
            // Таймер системного вызова, после каждой инструкции + 1
            unsigned syscall_timer = 0;

            // Флаг ошибки, должен обрабатываться при системном вызове
            // 0 - нет ошибки
            // 1 - ошибка сегментации
            // 3 - ошибка регистра
            // 4 - ошибка указателя текущей инструкции
            // 5 - ошибка инструкции (неверная инструкция)
            int err_flag = 0;

            // Таблица прерываний
            int intr_table[16];

            const std::size_t memory_size = 4096;

            // Оперативная память
            memory RAM;

            // Устройства подключённые к процессору
            std::vector<std::unique_ptr<utility_units::virtual_port>> ports;

            // Инструкции для работы с памятью

                // Загрузить из ОЗУ в регистр, адрес - константа, при safe_address_mode проверяет на доступность адреса
                // accumulator - адрес регистра куда сохранить
                // static_adress - адрес откуда брать
                void lodi(raddr accumulator, std::size_t static_adress) {
                    if (safe_address_mode) {
                        if (static_adress >= memory_addres_min and static_adress <= memory_addres_max) {
                            registers[accumulator] = RAM.get_from_memory(static_adress);
                        } else {
                            err_flag = 1;
                        }
                    } else {
                        registers[accumulator] = RAM.get_from_memory(static_adress);
                    }
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Загрузить из ОЗУ в регистр, адрес берётся из регистра, при safe_address_mode проверяет на доступность адреса
                // accumulator - адрес регистра куда сохранить
                // reg_addressator - адрес регистра хранящего адрес откуда брать
                void lodr(raddr accumulator, raddr reg_addressator) {
                    if (safe_address_mode) {
                        if (registers[reg_addressator] >= memory_addres_min and registers[reg_addressator] <= memory_addres_max) {
                            registers[accumulator] = RAM.get_from_memory(registers[reg_addressator]);
                        } else {
                            err_flag = 1;
                        }
                    } else {
                        registers[accumulator] = RAM.get_from_memory(registers[reg_addressator]);
                    }
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Запись в ОЗУ по статичному адресу
                // static_adress - адрес
                // reg - адрес регистра где хранится значение
                void stri(std::size_t static_adress, raddr reg) {
                    if (safe_address_mode) {
                        if (static_adress >= memory_addres_min and static_adress <= memory_addres_max) {
                            RAM.set_to_memory(static_adress, registers[reg]);
                        } else {
                            err_flag = 3;
                        }
                    } else {
                        RAM.set_to_memory(static_adress, registers[reg]);
                    }
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Запись в ОЗУ по статичному адресу
                // reg_addressator - адрес регистра, где хранится адрес
                // reg - адрес регистра где хранится значение
                void strr(raddr reg_addressator, raddr reg) {
                    if (safe_address_mode) {
                        if (registers[reg_addressator] >= memory_addres_min and registers[reg_addressator] <= memory_addres_max) {
                            RAM.set_to_memory(registers[reg_addressator], registers[reg]);
                        } else {
                            err_flag = 3;
                        }
                    } else {
                        RAM.set_to_memory(registers[reg_addressator], registers[reg]);
                    }
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Установка максимального и минимального адреса
                // reg_min - адресс регистра хранящего минимальный адрес
                // reg_max - адресс регистра хранящего максимальный адрес
                void amin(raddr reg_min, raddr reg_max) {
                    memory_addres_min = registers[reg_min];
                    memory_addres_max = registers[reg_max];
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Включение проверки адреса
                void setl() {
                    safe_address_mode = true;
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Выключение проверки адреса
                void setf() {
                    safe_address_mode = false;
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

            // Арифметические инструкции

                // Инструкция сложения
                // accumulator - адрес регистра результата
                // sum1 - адрес регистра первого слагаемого
                // sum2 - адрес регистра второго слагаемого
                void add(raddr accumulator, raddr sum1, raddr sum2) {
                    if (check_reg_addr(accumulator) or check_reg_addr(sum1) or check_reg_addr(sum2)) {return;}
                    registers[accumulator] = registers[sum1] + registers[sum2];
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Инструкция сложения с константой
                // accumulator - адрес регистра результата
                // sum1 - адрес регистра первого слагаемого
                // value - значение
                void add(raddr accumulator, raddr sum1, int value) {
                    if (check_reg_addr(accumulator) or check_reg_addr(sum1)) {return;}
                    registers[accumulator] = registers[sum1] + value;
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Запись в регистр константы
                // accumulator - адрес регистра назначения
                // value - значение
                void loc(raddr accumulator, int value) {
                    if (check_reg_addr(accumulator)) {return;}
                    registers[accumulator] = value;
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Инструкция вычитания
                // accumulator - адрес регистра результата
                // sub1 - адрес регистра первого слагаемого
                // sub2 - адрес регистра второго слагаемого
                void sub(raddr accumulator, raddr sub1, raddr sub2) {
                    if (check_reg_addr(accumulator) or check_reg_addr(sub1) or check_reg_addr(sub2)) {return;}
                    registers[accumulator] = registers[sub1] - registers[sub2];
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Инструкция умножения
                // accumulator - адрес регистра результата
                // mult1 - адрес регистра первого слагаемого
                // mult2 - адрес регистра второго слагаемого
                void mult(raddr accumulator, raddr mult1, raddr mult2) {
                    if (check_reg_addr(accumulator) or check_reg_addr(mult1) or check_reg_addr(mult2)) {return;}
                    registers[accumulator] = registers[mult1] * registers[mult2];
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Инструкция деления, ложит в аккумулятор частное
                // accumulator - адрес регистра результата
                // div1 - адрес регистра первого слагаемого
                // div2 - адрес регистра второго слагаемого
                void div(raddr accumulator, raddr div1, raddr div2) {
                    if (check_reg_addr(accumulator) or check_reg_addr(div1) or check_reg_addr(div2)) {return;}
                    registers[accumulator] = registers[div1] / registers[div2];
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Инструкция остатка деления, ложит в аккумулятор остаток
                // accumulator - адрес регистра результата
                // mod1 - адрес регистра первого слагаемого
                // mod2 - адрес регистра второго слагаемого
                void mod(raddr accumulator, raddr mod1, raddr mod2) {
                    if (check_reg_addr(accumulator) or check_reg_addr(mod1) or check_reg_addr(mod2)) {return;}
                    registers[accumulator] = registers[mod1] % registers[mod2];
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

            // Условные переходы и сравнения
                // Сравнение значений
                // reg1 - адрес регистра первого значения
                // reg2 - адрес регистра второго значения
                // = 0
                // > 1
                // < -1
                void cmp(raddr reg1, raddr reg2) {
                    if (check_reg_addr(reg1) or check_reg_addr(reg2)) {return;}
                    if (registers[reg1] == registers[reg2]) {
                        cmp_flag = 0;
                    } else if (registers[reg1] > registers[reg2]) {
                        cmp_flag = 1;
                    } else {
                        cmp_flag = -1;
                    }
                    registers[14] += 4; // Увеличиваем указатель инструкции на шаг
                }

                // Условный переход
                // condition - условие (= 0; > 1; < -1; >= 2; <= -2; != 3)
                // gotoaddr - адресс перехода
                void jmp(int condition, std::size_t gotoaddr) {
                    if (cmp_flag == condition) {
                        registers[14] = gotoaddr;
                        return;
                    } else if ((cmp_flag == 1 or cmp_flag == 0) and condition == 2) {
                        registers[14] = gotoaddr;
                        return;
                    } else if ((cmp_flag == -1 or cmp_flag == 0) and condition == -2) {
                        registers[14] = gotoaddr;
                        return;
                    } else if (cmp_flag != 0 and condition == 3) {
                        registers[14] = gotoaddr;
                        return;
                    }
                    registers[14] += 4;

                }

                // Безусловный переход
                // gotoaddr - адрес перехода
                void gotop(std::size_t gotoaddr) {
                    registers[14] = gotoaddr;
                }

        public:
            void init(std::vector<int> &program) {
                ports.push_back(std::make_unique<utility_units::terminal>);

            }


    };
}
