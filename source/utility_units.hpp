#pragma once

#include <fstream>
#include <iostream>

namespace utility_units {

    // Виртуальный класс порта
    class virtual_port {
    protected:
        // Состояние устройства, для каждого устройства свои коды
        int return_state = 0;
    public:
        virtual void send_value(int value) = 0;
        virtual void send_signal(int value) = 0;
        virtual void ret_value(int &answer) = 0;
        virtual void ret_signal(int &answer) = 0;
        ~virtual_port() = default;
    };

    // Класс наследник виртуального порта, позволяет работать с терминалом
    // При состоянии 0 - считывает и выводит символ
    // При состоянии 1 - считывает и выводит число
    class terminal : public virtual_port {
        void send_value(int value) override {
            if (return_state == 0) {std::cout << char(value);}
            else {std::cout << value;}
        }

        void send_signal(int value) override {
            return_state = value;
        }

        void ret_value(int &answer) override {
            if (return_state == 0) {
                char a;
                std::cin >> a;
                answer = a;
            } else {
                std::cin >> answer;
            }
        }

        void ret_signal(int &answer) override {
            answer = return_state;
        }
    };

    // Класс наследник виртуального порта, позволяет худо бедно работать с файловой системой
    // Если файл не открыт (mode = 0), то отправка данныъ по шине памяти будет расценена как запись имени файла
    // Если файл открыт в режиме чтения, то отправка данных по шине памяти закроет файл и в return_state будет ошибка
    // 0 - файл не открыт
    // 1 - файл открыт для чтения
    // 2 - файл открыт для записи
    // 3 - ошибка открытия файла
    // 4 - файл закрыт из-за попытки записи при режиме чтения или наоборот
    // 5 - неверная команда
    class fileunit : public virtual_port {
    protected:
        std::string filename;
        std::fstream f;
    public:
        // Приёмник контролирующих сигналов на порт устройства
        // Принимает сигнал управления открытия закрытия файла
        // 0 - закрыть файл
        // 1 - открыть для чтения
        // 2 - открыть для записи
        // иное значение приведёт к закрытию файла и ошибке
        void send_signal(int value) override {
            switch (value) {
                case 0: {
                    f.close();
                    filename.clear();
                    return_state = 0;
                    break;
                }
                case 1: {
                    if (f.is_open()) {
                        f.close();
                        filename.clear();
                        return_state = 4;
                    } else {
                        try {
                            f.open(filename, std::ios::in);
                            return_state = 1;
                        } catch (std::runtime_error &e) {
                            f.close();
                            filename.clear();
                            return_state = 3;
                        }
                    }
                    break;
                }
                case 2: {
                    if (f.is_open()) {
                        f.close();
                        filename.clear();
                        return_state = 4;
                    } else {
                        try {
                            f.open(filename, std::ios::out);
                            return_state = 2;
                        } catch (std::runtime_error &e) {
                            f.close();
                            filename.clear();
                            return_state = 3;
                        }
                    }
                    break;
                }
                default: {
                    f.close();
                    filename.clear();
                    return_state = 4;
                }
            }
        }

        // Запись в файл/имя файла, при попытке записи в режиме чтения всё упадёт в ошибку режима
        void send_value(int value) override {
            if (f.is_open()) {
                if (return_state == 2) {
                    f << char(value);
                } else {
                    return_state = 5;
                    f.close();
                    filename.clear();
                }
            } else {
                filename += char(value);
            }
        }

        // Чтение из файла, при попытке чтения в режиме записи падает, если файл не открыт то тоже всё роняет
        void ret_value(int &answer) override {
            if (f.is_open()) {
                if (return_state == 2) {
                    char c;
                    f.get(c);
                    answer = c;
                } else {
                    return_state = 5;
                    f.close();
                    filename.clear();
                }
            } else {
                return_state = 0;
                f.close();
                filename.clear();
            }
        }

        // Вернуть значение.
        void ret_signal(int &answer) override {
            answer = return_state;
        }

    };
}
