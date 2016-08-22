#include "test-sf.h"

#include <QCoreApplication>

int main(int argc, char **argv)
{
    QCoreApplication app(argc,argv);

    TestSF testsf;

    return app.exec();
}
