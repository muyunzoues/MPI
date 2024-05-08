#include <vector>
#include <iostream>
#include <mpi.h>

using namespace std;

// �鲢��������һ�� vector ������
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

// vec1 �� vec2 �������������飬����ϲ�Ϊһ����������
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
    // ��ǰ���̵ı��
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    // ��������
    MPI_Comm_size(MPI_COMM_WORLD, &processNum);
    // ������
    MPI_Get_processor_name(processor_name, &namelen);


    // ���ݵ�������Ĭ��Ϊ 10
    int numberNums = 10;
    // ��������в����ṩ��������������ʹ�������в���ָ��������
    if (argc > 1) {
        numberNums = atoi(argv[1]);
    }

    vector<int> vec;
    // ��ʼ������
    if (myrank == 0) {
        srand(time(NULL));
        // ������� numberNums ����
        for (int i = 0; i < numberNums; i++) {
            vec.push_back(rand() % 100);
        }
        cout << "��ʼ����:";
        for (int i = 0; i < vec.size(); i++) {
            cout << vec[i] << " ";
        }
        cout << endl;
    }

    // ÿ���������������
    int n = numberNums / processNum;
    // �ֳ�����������
    vector<int> subVec(n, 0);

    // ���㻮�ִ���
    int splitTimes = ceil(log2(processNum));

    if (myrank != 0) {
        // ���㵱ǰ���̵���һ������
        int preProcess = getPreProcess(myrank);
        // ��������Ĵ�С
        int vecNum;
        MPI_Recv(&vecNum, 1, MPI_INT, preProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        cout << "����" << myrank << " ������ " << vecNum << " �����֣��ӽ���" << preProcess << " ������Ϊ" << processor_name << endl;
        // ��������
        subVec = vector<int>(vecNum, 0);
        MPI_Recv(subVec.data(), vecNum, MPI_INT, preProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    for (int i = splitTimes; i > 0; i--) {
        // ��ʼ�� 0 �Ž��̵�������
        if (myrank == 0 && i == splitTimes)
            subVec = vector<int>(vec.begin(), vec.end());
        // �����ǰ������ 2 �� i �η��ı�������ô�ͷ�������
        if (myrank % int(pow(2, i)) == 0) {
            // ���㵱ǰ���̵���һ������
            int nextProcess = myrank + pow(2, i - 1);
            // �����һ�����̴��ڣ���ô�ͷ������ݸ���һ������
            if (nextProcess < processNum) {
                // ��������Ĵ�С
                int vecNum = subVec.size() - subVec.size() / 2;
                cout << "����" << myrank << " ������ " << vecNum << "�����ֵ�����" << nextProcess << " ������Ϊ" << processor_name << endl;
                MPI_Send(&vecNum, 1, MPI_INT, nextProcess, 0, MPI_COMM_WORLD);
                // ��������
                MPI_Send(subVec.data() + subVec.size() / 2, vecNum, MPI_INT, nextProcess, 0, MPI_COMM_WORLD);
                subVec = vector<int>(subVec.begin(), subVec.begin() + subVec.size() / 2);
            }
        }
    }

    // �Խ��յ����������������    
    mergeSort(subVec);

    // �ϲ�����
    // ����ϲ��Ĵ���
    int mergeTimes = ceil(log2(processNum));

    for (int i = 0; i < mergeTimes; i++) {
        // �жϵ�ǰ���� �� �� i ���ִ� �Ƿ���Ҫ��������
        if (myrank % int(pow(2, i + 1)) == 0) {
            // ���㵱ǰ���̵���һ������
            int nextProcess = myrank + pow(2, i);
            // �����һ�����̴��ڣ���ô�ͽ�����һ�����̵�����
            if (nextProcess < processNum) {
                // ��������Ĵ�С
                int vecNum;
                MPI_Recv(&vecNum, 1, MPI_INT, nextProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // ��������
                vector<int> recvVec(vecNum, 0);
                MPI_Recv(recvVec.data(), vecNum, MPI_INT, nextProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // �ϲ�����
                subVec = merge(subVec, recvVec);
            }
        }

        // �жϵ�ǰ���� �� �� i ���ִ� �Ƿ���Ҫ�������ݸ�ǰ�ߵĽ���
        if ((myrank + int(pow(2, i))) % int(pow(2, i + 1)) == 0) {
            // ������һ������
            int preProcess = myrank - pow(2, i);
            // ��������Ĵ�С
            int vecNum = subVec.size();
            MPI_Send(&vecNum, 1, MPI_INT, preProcess, 0, MPI_COMM_WORLD);
            // ��������
            MPI_Send(subVec.data(), subVec.size(), MPI_INT, preProcess, 0, MPI_COMM_WORLD);
        }
    }

    // �������������
    if (myrank == 0) {
        // �������������
        cout << "���������Ϊ��";
        for (int i = 0; i < subVec.size(); i++) {
            cout << subVec[i] << " ";
        }
        cout << endl;
    }

    MPI_Finalize();

    return 0;
}
