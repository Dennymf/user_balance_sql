#include <format>
#include <iostream>
#include <mysql.h>
#include <string>
#include <vector>

MYSQL* connection, mysql;
MYSQL_RES* result;
MYSQL_ROW row;
int query_state;
int counter = 1;

void get_all(std::string db, std::string table)
{
    std::string q1 = std::format("DESCRIBE {}.{}", db, table);
    query_state = mysql_query(connection, q1.c_str());
    if (query_state != 0) {
        std::cerr << mysql_error(connection) << std::endl;
    }
    result = mysql_store_result(connection);
    std::vector<std::string> row_name;
    while ((row = mysql_fetch_row(result)) != NULL) {
        row_name.push_back(row[0]);
    }

    std::string q = std::format("SELECT * FROM {}.{}", db, table);
    query_state = mysql_query(connection, q.c_str());
    if (query_state != 0) {
        std::cerr << mysql_error(connection) << std::endl;
    }

    result = mysql_store_result(connection);
    while ((row = mysql_fetch_row(result)) != NULL) {
        for (size_t i = 0; i < mysql_num_fields(result); ++i)
        {
            std::cout << row_name[i] << " : " << row[i] << '\n';
        }
    }
}

bool findById(std::string db, std::string table, int id)
{
    std::string q = std::format("SELECT * FROM {}.{} WHERE id={}", db, table, id);
    query_state = mysql_query(connection, q.c_str());
    if (query_state != 0) {
        std::cerr << mysql_error(connection) << std::endl;
    }

    result = mysql_store_result(connection);

    return mysql_fetch_row(result) != NULL;
}

bool checkPossibly(std::string db, std::string table, int id, int value)
{
    std::string q = std::format("SELECT `balance` FROM {}.{} WHERE `id`={}", db, table, id);
    query_state = mysql_query(connection, q.c_str());
    result = mysql_store_result(connection);
    auto row = mysql_fetch_row(result);
    return std::stoi(row[0]) >= abs(value);
}

void updateBalance(std::string db, std::string table, int id, int value)
{
    if (value < 0)
    {
        if (!checkPossibly(db, table, id, value))
        {
            std::cout << "Insufficient funds\n";
            return;
        }
    }
    std::string q = std::format("UPDATE {}.{} SET `balance`=`balance`+{} WHERE id={}", db, table, value, id);
    mysql_query(connection, q.c_str());
}

void addNewUser(std::string db, std::string table, int id, int value)
{
    std::string q = std::format("INSERT INTO {}.{} VALUES ({}, {})", db, table, id, value);
    mysql_query(connection, q.c_str());
}

int getBalance(std::string db, std::string table, int id)
{
    std::string q = std::format("SELECT `balance` FROM {}.{} WHERE `id`={}", db, table, id);
    query_state = mysql_query(connection, q.c_str());
    result = mysql_store_result(connection);
    auto row = mysql_fetch_row(result);

    return row != NULL ? std::stoi(row[0]) : 0;
}

void addOperaion(std::string db, std::string table, int id, int value, std::string operation, std::string time)
{
    std::string q = std::format("INSERT INTO {}.{} VALUES ({}, {}, {}, \"{}\", {})", db, table, counter, id, value, operation, time);
    counter++;
    mysql_query(connection, q.c_str());
}

int findMax(std::string db, std::string table)
{
    std::string q = std::format("SELECT MAX(id) FROM {}.{}", db, table);
    query_state = mysql_query(connection, q.c_str());
    result = mysql_store_result(connection);
    auto row = mysql_fetch_row(result);

    return std::stoi(row[0]);
}

void getOperation(std::string db, std::string table, int id)
{
    std::string q = std::format("SELECT * FROM {}.{} WHERE `userid`={}", db, table, id);
    query_state = mysql_query(connection, q.c_str());
    if (query_state != 0) {
        std::cerr << mysql_error(connection) << std::endl;
    }

    result = mysql_store_result(connection);
    while ((row = mysql_fetch_row(result)) != NULL) {
        std::cout << std::format("Number operation - {} : {} a {} in {}\n", row[0], row[3], row[2], row[4]);
    }
}

int main()
{
    mysql_init(&mysql);
    connection = mysql_real_connect(&mysql, "localhost", "name", "password", "user", 3306, 0, 0);
    if (connection == NULL) {
        std::cout << mysql_error(&mysql) << std::endl;
        return 1;
    }

    std::string db = "`my_db`";
    std::string table = "`user`";
    std::string table2 = "`operation`";
    get_all(db, table);
    counter = findMax(db, table2) + 1;
    while (true)
    {
        std::string comm;
        std::cin >> comm;
        if (comm == "add" || comm == "off")
        {
            int id;
            int value;
            std::cin >> id >> value;

            if (value < 0)
            {
                std::cout << "Expected unsigned balance\n";
            }
            else 
            {
                if (comm == "off")
                {
                    value *= -1;
                }

                if (findById(db, table, id))
                {
                    updateBalance(db, table, id, value);
                    if (value > 0)
                    { 
                        addOperaion(db, table2, id, +value, "add", "NOW()");
                    }
                    else 
                    {
                        addOperaion(db, table2, id, -value, "off", "NOW()");
                    }
                }
                else if(value > 0)
                {
                    addNewUser(db, table, id, value);
                    addOperaion(db, table2, id, +value, "add", "NOW()");
                }
            }
        }
        else if (comm == "balance")
        {
            int id;
            std::cin >> id;
            if (findById(db, table, id))
            {
                std::cout << std::format("Balance by ID:{} = {}\n", id, getBalance(db, table, id));
            }
            else
            {
                std::cout << std::format("Can't found user with ID = {}\n", id);
            }
        }
        else if (comm == "transfer")
        {
            int id1;
            int id2;
            int value;
            std::cin >> id1 >> id2 >> value;
            if (findById(db, table, id1))
            {
                if (checkPossibly(db, table, id1, value))
                {
                    updateBalance(db, table, id1, -value);
                    addOperaion(db, table2, id1, -value, "transfer to " + std::to_string(id2), "NOW()");
                    if (findById(db, table, id2))
                    {
                        updateBalance(db, table, id2, value);
                    }
                    else
                    {
                        addNewUser(db, table, id2, value);
                    }
                    addOperaion(db, table2, id2, +value, "transfer from " + std::to_string(id1), "NOW()");
                }
                else
                {
                    std::cout << std::format("Insufficient funds at Id = {}\n", id1);
                }
            }
            else
            {
                std::cout << std::format("Can't found user with ID = {}\n", id1);
            }
        }
        else if (comm == "operation")
        {
            int id;
            std::cin >> id;
            if (findById(db, table, id))
            {
                getOperation(db, table2, id);
            }
            else
            {
                std::cout << std::format("Insufficient funds at Id = {}\n", id);
            }
        }
        else
        {
            std::cout << "Unknow command\n";
        }
        get_all(db, table);
    }
    mysql_free_result(result);
    mysql_close(connection);
}