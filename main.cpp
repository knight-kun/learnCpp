#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <set>
#include <algorithm>
#include <unordered_set>
#include <memory> // 用于智能指针的例子
#include <utility>

#include "time_meter.h"

using namespace std;

// 某货物一批库存的信息
struct InvBatch
{
    size_t no = 0;                 // 编号
    unsigned int package_size = 0; // 最小包装大小
    unsigned int package_num = 0;  // 包装数量
    inline unsigned int GetWareNum() { return package_size * package_num; }
    InvBatch(unsigned int _package_size, unsigned int _package_num) : package_size(_package_size), package_num(_package_num) {}
    static void MakeNo(vector<InvBatch> &batchs)
    {
        size_t global_no = 0;
        for (auto &batch : batchs)
        {
            batch.no = global_no++;
        }
    }
};

// 某种出库组合的信息
struct ObInfo
{
    unsigned int ware_num = 0;                       // 当前组合货物数量
    list<pair<const InvBatch *, unsigned int>> info; // 组合信息
    ObInfo() {}
    ObInfo(const InvBatch *batch, unsigned int batch_package_num)
    {
        ware_num = batch->package_size * batch_package_num;
        info.push_back(make_pair(batch, batch_package_num));
    }
    ObInfo(const ObInfo &base_ob_info, const ObInfo &add_ob_info)
    {
        ware_num = base_ob_info.ware_num + add_ob_info.ware_num;
        info = base_ob_info.info;
        info.push_back(add_ob_info.info.front());
    }
    inline bool operator==(const ObInfo &other) const { return ware_num == other.ware_num; }
    inline bool operator<(const ObInfo &other) const { return ware_num < other.ware_num; }
    inline bool operator>(const ObInfo &other) const { return ware_num > other.ware_num; }
};

int main()
{
    // 算法参数
    const unsigned int optimal_ware_num_max = 500; // 可保证解最优的最大货物数量（精度）。此值越小计算速度越快，但无准确解的可能性更大，需通过实践确定最优参数。
    // TODO 除了精度外，还可以添加时间约束

    // 输入
    const unsigned int ob_order_num = 48000; // 本次订单出库数量
    vector<InvBatch> inv = {{12, 117},
                            {17, 81},
                            {29, 77},
                            {37, 73},
                            {7, 17},
                            {3, 184},
                            {55, 99},
                            {40, 78},
                            {77, 53},
                            {6, 375},
                            {13, 175},
                            {29, 113},
                            {17, 100},
                            {37, 100},
                            {7, 100},
                            {3, 100},
                            {77, 20},
                            {40, 50},
                            {55, 200}};
    InvBatch::MakeNo(inv);

    // 统计
    int inv_ware_num = 0;
    for (auto &batch : inv)
    {
        inv_ware_num += batch.GetWareNum();
    }
    cout << "inv_ware_num: " << inv_ware_num << endl;
    cout << "ob_order_num: " << ob_order_num << endl;
    cout << endl;

    if (ob_order_num == 0 || ob_order_num > inv_ware_num)
    {
        cout << "wrong order num!" << endl;
        return 0;
    }

    // 辅助结构
    TimeMeter tm;
    long long core_cnt = 0;  // 基础计算次数统计
    long long erase_cnt = 0; // 清楚无效ob_info统计
    double core_time = 0;
    double erase_time = 0;

    // TODO 若想进一步提速，可对ob_infos使用多索引队列：1. 递减排序便于从头部遍历；2. 哈希查找便于值更新
    set<ObInfo, greater<ObInfo>> ob_infos; // 当前所有的出库组合
    ob_infos.insert(ObInfo());             // 初始化

    // 逐batch处理库存
    for (auto &batch : inv)
    {
        auto cur_max_ware_num = ob_infos.begin()->ware_num; // 当前所有组合中最多的货物数量
        cout << "proc batch: " << batch.no << "\t time: " << tm.elapsed_s(false) << "\t cur max ware num: " << cur_max_ware_num << endl;

        /* 计算当前批次组合 */
        vector<ObInfo> batch_ob_infos; // 当前批次库存的所有的出库组合

        unsigned int package_cover_ob_num = (unsigned int)ceil((float)ob_order_num / batch.package_size); // 若此批货数量无限，需要多少package
        unsigned int batch_package_end = min(package_cover_ob_num, batch.package_num);                    // 与实际值取最小
        unsigned int batch_package_begin = 1;
        // 若此批货数量超精度，且增加此批货后仍未超订单数，仅考虑头部可能性
        if (batch.GetWareNum() > optimal_ware_num_max && cur_max_ware_num + batch.GetWareNum() < ob_order_num)
        {
            batch_package_begin = (batch.GetWareNum() - optimal_ware_num_max) / batch.package_size;
            batch_package_begin = max(batch_package_begin, (unsigned int)1);
        }
        batch_ob_infos.reserve(batch_package_end - batch_package_begin + 1);

        for (unsigned int cur_batch_package_num = batch_package_begin; cur_batch_package_num <= batch_package_end; ++cur_batch_package_num)
        {
            batch_ob_infos.push_back(ObInfo(&batch, cur_batch_package_num));
        }

        /* 更新出库组合 */
        for (auto &ob_info : ob_infos)
        {
            // 超容量组合无需更新
            if (ob_info.ware_num > ob_order_num)
                break;

            // 提速，只考虑头部货物数量的组合
            auto interval_end = min(cur_max_ware_num, ob_order_num); // 当前应考虑的区间
            if (interval_end >= ob_info.ware_num && interval_end - ob_info.ware_num > optimal_ware_num_max)
                break;

            for (auto &batch_ob_info : batch_ob_infos)
            {
                /* 检查与早退 */
                if (ob_info.ware_num + batch_ob_info.ware_num == ob_order_num)
                {
                    /* 精度设置下找到解 */
                    ObInfo goal_ob_info(ob_info, batch_ob_info);
                    cout << endl;
                    cout << "find result:" << endl;
                    unsigned int sum = 0;
                    for (auto &batch_info : goal_ob_info.info)
                    {
                        auto batch_ware_num = batch_info.first->package_size * batch_info.second;
                        sum += batch_ware_num;
                        cout << " no: " << batch_info.first->no;
                        cout << "\t " << (batch_info.second == batch_info.first->package_num ? "all " : "some") << " " << batch_info.second << "/" << batch_info.first->package_num;
                        cout << "\t package: " << batch_info.first->package_size << " * " << batch_info.second << " = " << batch_ware_num << " ";
                        cout << "\t sum_now: " << sum << endl;
                    }
                    cout << "cal " << core_cnt << " times" << endl;
                    cout << "erase " << erase_cnt << " times" << endl;
                    cout << "all_time: " << tm.elapsed_s() << endl;
                    cout << "core_time: " << core_time << endl;
                    cout << "erase_time: " << erase_time << endl;
                    return 0;
                }

                /* 更新 */
                TimeMeter tm_core;
                ob_infos.insert(ObInfo(ob_info, batch_ob_info)); // 如果已经存在结果，则insert无事发生
                ++core_cnt;
                core_time += tm_core.elapsed_s();
            }
        }

        /* 删除无效ob_info，减少内存 */
        TimeMeter tm_erase;
        cur_max_ware_num = ob_infos.begin()->ware_num;
        for (auto it = ob_infos.begin(); it != ob_infos.end();)
        {
            if (cur_max_ware_num - it->ware_num > optimal_ware_num_max)
            {
                it = ob_infos.erase(it);
                ++erase_cnt;
            }
            else
            {
                ++it;
            }
        }
        erase_time += tm_erase.elapsed_s();
    }

    /* 精度设置下未找到解 */
    cout << endl;
    cout << "dont find result" << endl;
    cout << "but have some possible result" << endl;
    cout << "cal " << core_cnt << " times" << endl;
    cout << "erase " << erase_cnt << " times" << endl;
    cout << "all_time: " << tm.elapsed_s() << endl;
    cout << "core_time: " << core_time << endl;
    cout << "erase_time: " << erase_time << endl;

    return 0;
}