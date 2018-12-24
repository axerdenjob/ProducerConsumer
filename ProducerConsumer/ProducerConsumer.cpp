// ***********************************************************************
// Assembly         : 
// Author           : Alexander
// Created          : 12-24-2018
//
// Last Modified By : Alexander
// Last Modified On : 12-24-2018
// ***********************************************************************
// <summary>Producer/Consumer sorting exercise. Comments generated with Ghostdoc and edited.</summary>
// ***********************************************************************

#include "ProducerConsumer.h"

using namespace std;

/// <summary>
/// Enum sorttype
/// </summary>
enum sorttype
{
    /// <summary>
    /// The countingsort
    /// </summary>
    countingsort,
    /// <summary>
    /// The insertionsort
    /// </summary>
    insertionsort
};

/// <summary>
/// Class CProducerShared.
/// </summary>
class CProducerShared
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="CProducerShared" /> class.
    /// </summary>
    /// <param name="strOut">The string out.</param>
    explicit CProducerShared(const string &strOut)
        :
        _fileOut(strOut)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="CProducerShared" /> class.
    /// </summary>
    ~CProducerShared()
    {
        _fileOut.close();
    }

    /// <summary>
    /// Get a job from the queue.
    /// </summary>
    /// <param name="bRun">Keep running?</param>
    /// <returns>String of characters to sort</returns>
    string GetJob(bool &bRun)
    {
        string str;
        {
            unique_lock<mutex> lock(_mutexQueue);
            _conditionQueue.wait(lock, [this, &bRun]() {return !this->_queueJobs.empty() || !bRun; });

            if (!bRun)
                return "";

            str = _queueJobs.front();
            _queueJobs.pop();
        }
        return str;
    }

    /// <summary>
    /// The conditionQueue
    /// </summary>
    condition_variable _conditionQueue;
    /// <summary>
    /// The qm
    /// </summary>
    mutex _mutexQueue;
    /// <summary>
    /// The qr
    /// </summary>
    queue<string> _queueJobs;
    /// <summary>
    /// The MTX file out
    /// </summary>
    mutex _mutexFileOut;
    /// <summary>
    /// The m of out
    /// </summary>
    ofstream _fileOut;
};

/// <summary>
/// Class CConsumer.
/// </summary>
class CConsumer
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="CConsumer" /> class.
    /// </summary>
    /// <param name="eSortType">Type of the e sort.</param>
    /// <param name="ps">The ps.</param>
    CConsumer(sorttype eSortType, CProducerShared* ps)
        :
        _eSortType(eSortType),
        _pShared(ps),
        _bRunning(true),
        _thread(ThreadProc, this)
    {
    }

    /// <summary>
    /// Stops this instance.
    /// </summary>
    void Stop()
    {
        _bRunning = false;
        _pShared->_conditionQueue.notify_all();
        if (_thread.joinable()) {
            _thread.join();
            _pShared->_conditionQueue.notify_all();
        }
    }

private:
    /// <summary>
    /// Threads the proc.
    /// </summary>
    /// <param name="pSelf">The p self.</param>
    static void ThreadProc(CConsumer* pSelf)
    {
        pSelf->InternalThreadProc();
    }

    /// <summary>
    /// Internals the thread proc.
    /// </summary>
    void InternalThreadProc()
    {
        while (_bRunning)
        {
            string strJob = _pShared->GetJob(_bRunning);

            if ("" == strJob)
                continue;

            vector<uint8_t> vectorProcess;
            for (auto &ch : strJob)
            {
                if (' ' == ch)
                    this_thread::sleep_for(chrono::milliseconds(1000));
                else
                    vectorProcess.push_back(ch);
            }

            if (countingsort == _eSortType)
                sort_counting(vectorProcess);
            else
                sort_bubble(vectorProcess);

            OutputJob(vectorProcess);
        }
    }

    /// <summary>
    /// Perform counting sort
    /// </summary>
    /// <param name="v">Input vector</param>
    static void sort_counting(vector<uint8_t> &v)
    {
        uint8_t anCountTable[256] = {};
        auto vb = v.begin();

        for (auto it = vb; it != v.end(); ++it)
        {
            anCountTable[*it]++;
        }

        for (int x = '0'; x <= '9'; ++x)
        {
            for (int y = 0; y < anCountTable[x]; ++y)
            {
                *vb++ = x;
            }
        }
    }

    /// <summary>
    /// Perform bubble sort
    /// </summary>
    /// <param name="v">Input vector</param>
    void sort_bubble(vector<uint8_t>& v)
    {
        for (auto i = v.begin(); i != v.end(); ++i)
            for (auto j = v.begin(); j < i; ++j)
                if (*i < *j)
                    iter_swap(i, j);
    }

    /// <summary>
    /// Outputs the job.
    /// </summary>
    /// <param name="v">Input vector</param>
    void OutputJob(vector<uint8_t> &v)
    {
        unique_lock<mutex> lock(_pShared->_mutexFileOut);
        for (auto ch : v)
        {
            _pShared->_fileOut << ch;
        }
        _pShared->_fileOut << endl;
    }

    /// <summary>
    /// The sort type
    /// </summary>
    sorttype _eSortType;
    /// <summary>
    /// Pointer to shared data
    /// </summary>
    CProducerShared* _pShared;
    /// <summary>
    /// Remain running?
    /// </summary>
    bool _bRunning;
    /// <summary>
    /// std thread
    /// </summary>
    thread _thread;
};

/// <summary>
/// Class CProducer.
/// </summary>
class CProducer
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="CProducer" /> class.
    /// </summary>
    /// <param name="strIn">Input file</param>
    /// <param name="strOut">Output file</param>
    /// <param name="eType">Sort type</param>
    CProducer(const string &strIn, const string &strOut, sorttype eType)
        :
        _InFile(strIn),
        _Shared(strOut)
    {
        for (int i = 0; i < 4; i++)
        {
            _vectorPool.push_back(new CConsumer(eType, &_Shared));
        }
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="CProducer" /> class.
    /// </summary>
    ~CProducer()
    {
        for (CConsumer* csm : _vectorPool)
        {
            csm->Stop();
        }

    }

    /// <summary>
    /// Processes the file.
    /// </summary>
    void ProcessFile()
    {
        string strLine;
        while (getline(_InFile, strLine))
        {
            Add_Job(strLine);
        }

        while (!_Shared._queueJobs.empty())
        {
            this_thread::yield();
        }
    }

private:
    /// <summary>
    /// Adds the job.
    /// </summary>
    /// <param name="str">The string.</param>
    void Add_Job(string strJob)
    {
        {
            unique_lock<mutex> lock(_Shared._mutexQueue);
            _Shared._queueJobs.push(strJob);
        }
        _Shared._conditionQueue.notify_one();
    }

    /// <summary>
    /// Input file
    /// </summary>
    ifstream _InFile;
    /// <summary>
    /// Thread pool
    /// </summary>
    vector<CConsumer*> _vectorPool;
    /// <summary>
    /// Shared data
    /// </summary>
    CProducerShared _Shared;
};

/// <summary>
/// Main proc
/// </summary>
/// <param name="argc">The argc.</param>
/// <param name="argv">The argv.</param>
/// <returns>int.</returns>
int main(int argc, char *argv[])
{
    try
    {
        CProducer pr(argv[1], argv[2], (0 == strcmp("countingsort", argv[3])) ? countingsort : insertionsort);
        pr.ProcessFile();
    }
    catch (const exception &e)
    {
        cout << e.what();

        return -1;
    }

    return 0;
}
