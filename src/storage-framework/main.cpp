#include "test-sf.h"

#include <QCoreApplication>

int main(int argc, char **argv)
{
    QCoreApplication app(argc,argv);

    bool commit = false;
    if (argc == 1)
    {
        commit = true;
    }
    TestSF testsf(commit);

    return app.exec();
}
