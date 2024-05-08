#include <vector>
#include <iostream>
#include <mpi.h>

using namespace std;

// 归并排序，输入一个 vector 的引用
void mergeSort(vector<int>& vec) {
    if (vec.size() <= 1) return;
    int mid = vec.size() / 2;
    vector<int> left(vec.begin(), vec.begin() + mid);
    vector<int> right(vec.begin() + mid, vec.end());
    mergeSort(left);
    mergeSort(right);
    int i = 0, j = 0, k = 0;
    while (i < left.size() && j < right.size()) {
        if (left[i] < right[j]) {
            vec[k++] = left[i++];
        }
        else {
            vec[k++] = right[j++];
        }
    }
    while (i < left.size()) {
        vec[k++] = left[i++];
    }
    while (j < right.size()) {
        vec[k++] = right[j++];
    }
}

// vec1 和 vec2 是两个有序数组，将其合并为一个有序数组
vector<int> merge(vector<int> vec1, vector<int> vec2) {
    int i = 0, j = 0;
    vector<int> res;
    while (i < vec1.size() && j < vec2.size()) {
        if (vec1[i] < vec2[j]) {
            res.push_back(vec1[i++]);
        }
        else {
            res.push_back(vec2[j++]);
        }
    }
    while (i < vec1.size()) {
        res.push_back(vec1[i++]);
    }
    while (j < vec2.size()) {
        res.push_back(vec2[j++]);
    }
    return res;
}

int getPreProcess(int curProcessNum) {
    int tmp;
    int sum = 0;
    while (sum < curProcessNum) {
        tmp = 2;
        bool flag = false;
        while (sum + tmp < curProcessNum) {
            tmp *= 2;
        }
        if (tmp == 2 || sum + tmp == curProcessNum) {
            return sum;
        }
        else {
            sum += tmp / 2;
        }
    }
}

int main(int argc, char* argv[]) {
    int myrank, processNum;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int namelen;

    MPI_Init(&argc, &argv);
    // 当前进程的编号
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    // 进程总数
    MPI_Comm_size(MPI_COMM_WORLD, &processNum);
    // 机器名
    MPI_Get_processor_name(processor_name, &namelen);


    // 数据的数量，默认为 10
    int numberNums = 10;
    // 如果命令行参数提供了数据数量，则使用命令行参数指定的数量
    if (argc > 1) {
        numberNums = atoi(argv[1]);
    }

    vector<int> vec;
    // 初始化数据
    if (myrank == 0) {
        srand(time(NULL));
        // 随机生成 numberNums 个数
        for (int i = 0; i < numberNums; i++) {
            vec.push_back(rand() % 100);
        }
        cout << "初始数组:";
        for (int i = 0; i < vec.size(); i++) {
            cout << vec[i] << " ";
        }
        cout << endl;
    }

    // 每个进程排序的数量
    int n = numberNums / processNum;
    // 分出来的子数字
    vector<int> subVec(n, 0);

    // 计算划分次数
    int splitTimes = ceil(log2(processNum));

    if (myrank != 0) {
        // 计算当前进程的上一个进程
        int preProcess = getPreProcess(myrank);
        // 接收数组的大小
        int vecNum;
        MPI_Recv(&vecNum, 1, MPI_INT, preProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        cout << "进程" << myrank << " 接受了 " << vecNum << " 个数字，从进程" << preProcess << " 机器名为" << processor_name << endl;
        // 接收数组
        subVec = vector<int>(vecNum, 0);
        MPI_Recv(subVec.data(), vecNum, MPI_INT, preProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    for (int i = splitTimes; i > 0; i--) {
        // 初始化 0 号进程的子数组
        if (myrank == 0 && i == splitTimes)
            subVec = vector<int>(vec.begin(), vec.end());
        // 如果当前进程是 2 的 i 次方的倍数，那么就发送数据
        if (myrank % int(pow(2, i)) == 0) {
            // 计算当前进程的下一个进程
            int nextProcess = myrank + pow(2, i - 1);
            // 如果下一个进程存在，那么就发送数据给下一个进程
            if (nextProcess < processNum) {
                // 发送数组的大小
                int vecNum = subVec.size() - subVec.size() / 2;
                cout << "进程" << myrank << " 发送了 " << vecNum << "个数字到进程" << nextProcess << " 机器名为" << processor_name << endl;
                MPI_Send(&vecNum, 1, MPI_INT, nextProcess, 0, MPI_COMM_WORLD);
                // 发送数组
                MPI_Send(subVec.data() + subVec.size() / 2, vecNum, MPI_INT, nextProcess, 0, MPI_COMM_WORLD);
                subVec = vector<int>(subVec.begin(), subVec.begin() + subVec.size() / 2);
            }
        }
    }

    // 对接收到的子数组进行排序    
    mergeSort(subVec);

    // 合并数组
    // 计算合并的次数
    int mergeTimes = ceil(log2(processNum));

    for (int i = 0; i < mergeTimes; i++) {
        // 判断当前进程 在 第 i 个轮次 是否需要接收数据
        if (myrank % int(pow(2, i + 1)) == 0) {
            // 计算当前进程的下一个进程
            int nextProcess = myrank + pow(2, i);
            // 如果下一个进程存在，那么就接收下一个进程的数据
            if (nextProcess < processNum) {
                // 接收数组的大小
                int vecNum;
                MPI_Recv(&vecNum, 1, MPI_INT, nextProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // 接收数组
                vector<int> recvVec(vecNum, 0);
                MPI_Recv(recvVec.data(), vecNum, MPI_INT, nextProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // 合并数组
                subVec = merge(subVec, recvVec);
            }
        }

        // 判断当前进程 在 第 i 个轮次 是否需要发送数据给前边的进程
        if ((myrank + int(pow(2, i))) % int(pow(2, i + 1)) == 0) {
            // 计算上一个进程
            int preProcess = myrank - pow(2, i);
            // 发送数组的大小
            int vecNum = subVec.size();
            MPI_Send(&vecNum, 1, MPI_INT, preProcess, 0, MPI_COMM_WORLD);
            // 发送数组
            MPI_Send(subVec.data(), subVec.size(), MPI_INT, preProcess, 0, MPI_COMM_WORLD);
        }
    }

    // 输出排序后的数组
    if (myrank == 0) {
        // 输出排序后的数组
        cout << "排序后数组为：";
        for (int i = 0; i < subVec.size(); i++) {
            cout << subVec[i] << " ";
        }
        cout << endl;
    }

    MPI_Finalize();

    return 0;
}
