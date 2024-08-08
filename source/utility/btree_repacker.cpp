#include "StarBTreeDatabase.hpp"
#include "StarTime.hpp"
#include "StarFile.hpp"
#include "StarVersionOptionParser.hpp"

using namespace Star;

int main(int argc, char** argv) {
  try {
    double startTime = Time::monotonicTime();

    VersionOptionParser optParse;
    optParse.setSummary("Repacks a Starbound BTree file to shrink its file size");
    optParse.addArgument("input file path", OptionParser::Required, "Path to the BTree to be repacked");
    optParse.addArgument("output filename", OptionParser::Optional, "Output BTree file");

    auto opts = optParse.commandParseOrDie(argc, argv);

    String bTreePath = opts.arguments.at(0);
    String outputFilename = opts.arguments.get(1, bTreePath + ".repack");

    outputFilename = File::relativeTo(File::fullPath(File::dirName(outputFilename)), File::baseName(outputFilename));
    //open the old db
    BTreeDatabase db;
    db.setIODevice(File::open(bTreePath, IOMode::Read));
    db.open();

    //make a new db
    BTreeDatabase newDb;
    newDb.setBlockSize(db.blockSize());
    newDb.setContentIdentifier(db.contentIdentifier());
    newDb.setKeySize(db.keySize());
    newDb.setAutoCommit(false);

    newDb.setIODevice(File::open(outputFilename, IOMode::ReadWrite | IOMode::Truncate));
    newDb.open();
    coutf("Repacking {}...\n", bTreePath);
    //copy the data over
    unsigned count = 0, overwritten = 0;
    auto visitor = [&](ByteArray key, ByteArray data) {
      if (newDb.insert(key, data))
        ++overwritten;
      ++count;
    };
    auto errorHandler = [&](String const& error, std::exception const& e) {
      coutf("{}: {}\n", error, e.what());
    };
    db.recoverAll(visitor, errorHandler);

    //close the old db
    db.close();
    //commit and close the new db
    newDb.commit();
    newDb.close();

    coutf("Repacked BTree to {} in {:.6f}s\n({} inserts, {} overwritten)\n", outputFilename, Time::monotonicTime() - startTime, count, overwritten);
    return 0;

  } catch (std::exception const& e) {
    cerrf("Exception caught: {}\n", outputException(e, true));
    return 1;
  }
}
