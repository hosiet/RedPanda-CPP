#include "debugger.h"
#include "utils.h"
#include "mainwindow.h"
#include "editor.h"
#include "settings.h"
#include "cpudialog.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QMessageBox>

Debugger::Debugger(QObject *parent) : QObject(parent)
{
    mBreakpointModel=new BreakpointModel(this);
    mBacktraceModel=new BacktraceModel(this);
    mExecuting = false;
    mUseUTF8 = false;
    mReader = nullptr;
    mCommandChanged = false;
    mLeftPageIndexBackup = -1;
}

void Debugger::start()
{
    Settings::PCompilerSet compilerSet = pSettings->compilerSets().defaultSet();
    if (!compilerSet) {
        QMessageBox::critical(pMainWindow,
                              tr("No compiler set"),
                              tr("No compiler set is configured.")+tr("Can't start debugging."));
        return;
    }
    mExecuting = true;
    QString debuggerPath = compilerSet->debugger();
    QFile debuggerProgram(debuggerPath);
    if (!debuggerProgram.exists()) {
        QMessageBox::critical(pMainWindow,
                              tr("Debugger not exists"),
                              tr("Can''t find debugger in : \"%1\"").arg(debuggerPath));
        return;
    }
    mReader = new DebugReader(this);
    mReader->setDebuggerPath(debuggerPath);
    connect(mReader, &QThread::finished,this,&Debugger::clearUpReader);
    connect(mReader, &DebugReader::parseFinished,this,&Debugger::syncFinishedParsing,Qt::BlockingQueuedConnection);
    connect(mReader, &DebugReader::changeDebugConsoleLastLine,this,&Debugger::onChangeDebugConsoleLastline,Qt::BlockingQueuedConnection);
    mReader->start();
    mReader->mStartSemaphore.acquire(1);

//fProcessID := pi.hProcess;

//// Create a thread that will read GDB output.
//Reader := TDebugReader.Create(true);
//Reader.PipeRead := fOutputRead;
//Reader.PipeWrite := fInputWrite;
//Reader.FreeOnTerminate := true;
//Reader.BreakpointList := BreakPointList;
//Reader.WatchVarList := WatchVarList;
//Reader.WatchView := WatchView;
//Reader.UseUTF8 := UseUTF8;
//Reader.Resume;
//Reader.OnInvalidateAllVars := OnInvalidateAllVars;

    pMainWindow->updateAppTitle();

    //Application.HintHidePause := 5000;
}
void Debugger::stop() {
    if (mExecuting) {
        mReader->stopDebug();
    }
}
void Debugger::clearUpReader()
{
    if (mExecuting) {
        mExecuting = false;

        //stop debugger
        mReader->deleteLater();
        mReader=nullptr;

//        if WatchVarList.Count = 0 then // nothing worth showing, restore view
//          MainForm.LeftPageControl.ActivePageIndex := LeftPageIndexBackup;

//        // Close CPU window
//        if Assigned(CPUForm) then
//          CPUForm.Close;

        // Free resources
        pMainWindow->removeActiveBreakpoints();

        pMainWindow->updateAppTitle();

        mBacktraceModel->clear();

//        Application.HintHidePause := 2500;

//        WatchView.Items.BeginUpdate;
//        try
//          //Clear all watch values
//          for I := 0 to WatchVarList.Count - 1 do begin
//            WatchVar := PWatchVar(WatchVarList.Items[I]);
//            WatchVar^.Node.Text := WatchVar^.Name + ' = '+Lang[ID_MSG_EXECUTE_TO_EVALUATE];

//            // Delete now invalid children
//            WatchVar^.Node.DeleteChildren;
//          end;
//        finally
//        WatchView.Items.EndUpdate;
//        end;
    }
}

void Debugger::sendCommand(const QString &command, const QString &params, bool updateWatch, bool showInConsole, DebugCommandSource source)
{
    if (mExecuting && mReader) {
        mReader->postCommand(command,params,updateWatch,showInConsole,source);
    }
}

void Debugger::addBreakpoint(int line, const Editor* editor)
{
    addBreakpoint(line,editor->filename());
}

void Debugger::addBreakpoint(int line, const QString &filename)
{
    PBreakpoint bp=std::make_shared<Breakpoint>();
    bp->line = line;
    bp->filename = filename;
    bp->condition = "";
    mBreakpointModel->addBreakpoint(bp);
    if (mExecuting) {
        sendBreakpointCommand(bp);
    }
}

void Debugger::deleteBreakpoints(const QString &filename)
{
    for (int i=mBreakpointModel->breakpoints().size()-1;i>=0;i--) {
        PBreakpoint bp = mBreakpointModel->breakpoints()[i];
        if (bp->filename == filename) {
            mBreakpointModel->removeBreakpoint(i);
        }
    }
}

void Debugger::deleteBreakpoints(const Editor *editor)
{
    deleteBreakpoints(editor->filename());
}

void Debugger::removeBreakpoint(int line, const Editor *editor)
{
    removeBreakpoint(line,editor->filename());
}

void Debugger::removeBreakpoint(int line, const QString &filename)
{
    for (int i=mBreakpointModel->breakpoints().size()-1;i>=0;i--) {
        PBreakpoint bp = mBreakpointModel->breakpoints()[i];
        if (bp->filename == filename && bp->line == line) {
            removeBreakpoint(i);
        }
    }
}

void Debugger::removeBreakpoint(int index)
{
    sendClearBreakpointCommand(index);
    mBreakpointModel->removeBreakpoint(index);
}

void Debugger::setBreakPointCondition(int index, const QString &condition)
{
    PBreakpoint breakpoint=mBreakpointModel->setBreakPointCondition(index,condition);
    if (condition.isEmpty()) {
        sendCommand("cond",
                    QString("%1").arg(breakpoint->line));
    } else {
        sendCommand("cond",
                    QString("%1 %2").arg(breakpoint->line).arg(condition));
    }
}

void Debugger::sendAllBreakpointsToDebugger()
{
    for (PBreakpoint breakpoint:mBreakpointModel->breakpoints()) {
        sendBreakpointCommand(breakpoint);
    }
}

void Debugger::addWatchVar(int i)
{
    //todo
}

void Debugger::removeWatchVar(int i)
{
    //todo
}

void Debugger::addWatchVar(const QString &namein)
{
    //todo
}

void Debugger::renameWatchVar(const QString &oldname, const QString &newname)
{
    //todo
}

void Debugger::refreshWatchVars()
{
    //todo
}

void Debugger::deleteWatchVars(bool deleteparent)
{
    //todo
}

void Debugger::invalidateAllVars()
{
    //todo
}

void Debugger::sendAllWatchvarsToDebugger()
{
    //todo
}

void Debugger::invalidateWatchVar(PWatchVar var)
{
    //toto
}

void Debugger::updateDebugInfo()
{
    sendCommand("backtrace", "");
    sendCommand("info locals", "");
    sendCommand("info args", "");
}

bool Debugger::useUTF8() const
{
    return mUseUTF8;
}

void Debugger::setUseUTF8(bool useUTF8)
{
    mUseUTF8 = useUTF8;
}

BacktraceModel* Debugger::backtraceModel()
{
    return mBacktraceModel;
}

BreakpointModel *Debugger::breakpointModel()
{
    return mBreakpointModel;
}

void Debugger::sendBreakpointCommand(int index)
{
    sendBreakpointCommand(mBreakpointModel->breakpoints()[index]);
}

void Debugger::sendBreakpointCommand(PBreakpoint breakpoint)
{
    if (breakpoint && mExecuting) {
        // break "filename":linenum
        QString condition;
        if (!breakpoint->condition.isEmpty()) {
            condition = " if " + breakpoint->condition;
        }
        QString filename = breakpoint->filename;
        filename.replace('\\','/');
        sendCommand("break",
                    QString("\"%1\":%2").arg(filename)
                    .arg(breakpoint->line)+condition);
    }
}

void Debugger::sendClearBreakpointCommand(int index)
{
    sendClearBreakpointCommand(mBreakpointModel->breakpoints()[index]);
}

void Debugger::sendClearBreakpointCommand(PBreakpoint breakpoint)
{
    // Debugger already running? Remove it from GDB
    if (breakpoint && mExecuting) {
        //clear "filename":linenum
        QString filename = breakpoint->filename;
        filename.replace('\\','/');
        sendCommand("clear",
                QString("\"%1\":%2").arg(filename)
                .arg(breakpoint->line));
    }
}

void Debugger::syncFinishedParsing()
{
    bool spawnedcpuform = false;

    // GDB determined that the source code is more recent than the executable. Ask the user if he wants to rebuild.
    if (mReader->doreceivedsfwarning) {
        if (QMessageBox::question(pMainWindow,
                                  tr("Compile"),
                                  tr("Source file is more recent than executable.")+"<BR /><BR />" + tr("Recompile?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes
                                  ) == QMessageBox::Yes) {
            stop();
            pMainWindow->compile();
            return;
        }
    }

    // The program to debug has stopped. Stop the debugger
    if (mReader->doprocessexited) {
        stop();
        return;
    }

    // An evaluation variable has been processed. Forward the results
    if (mReader->doevalready) {
        emit evalReady(mReader->mEvalValue);
    }

    // show command output
    if (pSettings->debugger().showCommandLog() ||
            (mReader->mCurrentCmd && mReader->mCurrentCmd->showInConsole)) {
        if (pSettings->debugger().showAnnotations()) {
            QString strOutput = mReader->mOutput;
            strOutput.replace(QChar(26),'>');
            pMainWindow->addDebugOutput(strOutput);
            pMainWindow->addDebugOutput("");
            pMainWindow->addDebugOutput("");
        } else {
            QStringList strList = TextToLines(mReader->mOutput);
            QStringList outStrList;
            bool addToLastLine=false;
            for (int i=0;i<strList.size();i++) {
                QString strOutput=strList[i];
                if (strOutput.startsWith("\032\032")) {
                    addToLastLine = true;
                } else {
                    if (addToLastLine && outStrList.size()>0) {
                        outStrList[outStrList.size()-1]+=strOutput;
                    } else {
                        outStrList.append(strOutput);
                    }
                    addToLastLine = false;
                }
            }
            for (const QString& line:outStrList) {
                pMainWindow->addDebugOutput(line);
            }
        }
    }

    // Some part of the CPU form has been updated
    if (pMainWindow->cpuDialog()->isVisible() && !mReader->doreceivedsignal) {
//        if (mReader->doregistersready)
//            CPUForm.OnRegistersReady;

//        if (mReader->dodisassemblerready)
//            CPUForm.OnAssemblerReady;
    }

//if dobacktraceready then
//  MainForm.OnBacktraceReady;


    if (mReader->doupdateexecution) {
        if (mReader->mCurrentCmd && mReader->mCurrentCmd->source == DebugCommandSource::Console) {
            pMainWindow->setActiveBreakpoint(mReader->mBreakPointFile, mReader->mBreakPointLine,false);
        } else {
            pMainWindow->setActiveBreakpoint(mReader->mBreakPointFile, mReader->mBreakPointLine);
        }
        refreshWatchVars(); // update variable information
    }

    if (mReader->doreceivedsignal) {
//SignalDialog := CreateMessageDialog(fSignal, mtError, [mbOk]);
//SignalCheck := TCheckBox.Create(SignalDialog);

//// Display it on top of everything
//SignalDialog.FormStyle := fsStayOnTop;

//SignalDialog.Height := 150;

//with SignalCheck do begin
//  Parent := SignalDialog;
//  Caption := 'Show CPU window';
//  Top := Parent.ClientHeight - 22;
//  Left := 8;
//  Width := Parent.ClientWidth - 16;
//  Checked := devData.ShowCPUSignal;
//end;

//MessageBeep(MB_ICONERROR);
//if SignalDialog.ShowModal = ID_OK then begin
//  devData.ShowCPUSignal := SignalCheck.Checked;
//  if SignalCheck.Checked and not Assigned(CPUForm) then begin
//    MainForm.ViewCPUItemClick(nil);
//    spawnedcpuform := true;
//  end;
//end;

//SignalDialog.Free;

    }


    // CPU form updates itself when spawned, don't update twice!
    if ((mReader->doupdatecpuwindow && !spawnedcpuform) && (pMainWindow->cpuDialog()->isVisible())) {
            sendCommand("disas", "");
            sendCommand("info registers", "");
    }
}

void Debugger::onChangeDebugConsoleLastline(const QString &text)
{
    //pMainWindow->changeDebugOutputLastline(text);
    pMainWindow->addDebugOutput(text);
}

int Debugger::leftPageIndexBackup() const
{
    return mLeftPageIndexBackup;
}

void Debugger::setLeftPageIndexBackup(int leftPageIndexBackup)
{
    mLeftPageIndexBackup = leftPageIndexBackup;
}

bool Debugger::executing() const
{
    return mExecuting;
}

DebugReader::DebugReader(Debugger* debugger, QObject *parent) : QThread(parent),
    mStartSemaphore(0)
{
    mDebugger = debugger;
    mProcess = nullptr;
    mUseUTF8 = false;
}

void DebugReader::postCommand(const QString &Command, const QString &Params, bool UpdateWatch, bool ShowInConsole, DebugCommandSource Source)
{
    QMutexLocker locker(&mCmdQueueMutex);
    if (mCmdQueue.isEmpty() && UpdateWatch) {
        emit pauseWatchUpdate();
        mUpdateCount++;
    }
    PDebugCommand pCmd = std::make_shared<DebugCommand>();
    pCmd->command = Command;
    pCmd->params = Params;
    pCmd->updateWatch = UpdateWatch;
    pCmd->showInConsole = ShowInConsole;
    pCmd->source = Source;
    mCmdQueue.enqueue(pCmd);
//    if (!mCmdRunning)
//        runNextCmd();
}

void DebugReader::clearCmdQueue()
{
    QMutexLocker locker(&mCmdQueueMutex);
    mCmdQueue.clear();

    if (mUpdateCount>0) {
        emit updateWatch();
        mUpdateCount=0;
    }
}

bool DebugReader::findAnnotation(AnnotationType annotation)
{
    AnnotationType NextAnnotation;
    do {
        NextAnnotation = getNextAnnotation();
        if (NextAnnotation == AnnotationType::TEOF)
            return false;
    } while (NextAnnotation != annotation);

    return true;
}

AnnotationType DebugReader::getAnnotation(const QString &s)
{
    if (s == "pre-prompt") {
        return AnnotationType::TPrePrompt;
    } else if (s == "prompt") {
        return AnnotationType::TPrompt;
    } else if (s == "post-prompt") {
        AnnotationType result = AnnotationType::TPostPrompt;

        int IndexBackup = mIndex;
        QString t = getNextFilledLine();
        mIndex = IndexBackup;
        //hack to catch local
        if ((mCurrentCmd) && (mCurrentCmd->command == "info locals")) {
            result = AnnotationType::TLocal;
        } else if ((mCurrentCmd) && (mCurrentCmd->command == "info args")) {
            //hack to catch params
            result = AnnotationType::TParam;
        } else if (t.startsWith("rax ") || t.startsWith("eax ")) {
            // Hack fix to catch register dump
            result = AnnotationType::TInfoReg;
        } else {
            // Another hack to catch assembler
            if (t.startsWith("Dump of assembler code for function "))
            result = AnnotationType::TInfoAsm;
        }
        return result;
    } else if (s == "error-begin") {
        return AnnotationType::TErrorBegin;
    } else if (s == "error-end") {
      return AnnotationType::TErrorEnd;
    } else if (s == "display-begin") {
      return AnnotationType::TDisplayBegin;
    } else if (s == "display-expression") {
      return AnnotationType::TDisplayExpression;
    } else if (s == "display-end") {
      return AnnotationType::TDisplayEnd;
    } else if (s == "frame-source-begin") {
      return AnnotationType::TFrameSourceBegin;
    } else if (s == "frame-source-file") {
      return AnnotationType::TFrameSourceFile;
    } else if (s == "frame-source-line") {
      return AnnotationType::TFrameSourceLine;
    } else if (s == "frame-function-name") {
      return AnnotationType::TFrameFunctionName;
    } else if (s == "frame-args") {
      return AnnotationType::TFrameArgs;
    } else if (s == "frame-begin") {
      return AnnotationType::TFrameBegin;
    } else if (s == "frame-end") {
      return AnnotationType::TFrameEnd;
    } else if (s == "frame-where") {
      return AnnotationType::TFrameWhere;
    } else if (s == "source") {
      return AnnotationType::TSource;
    } else if (s == "exited") {
      return AnnotationType::TExit;
    } else if (s == "arg-begin") {
      return AnnotationType::TArgBegin;
    } else if (s == "arg-name-end") {
      return AnnotationType::TArgNameEnd;
    } else if (s == "arg-value") {
      return AnnotationType::TArgValue;
    } else if (s == "arg-end") {
      return AnnotationType::TArgEnd;
    } else if (s == "array-section-begin") {
      return AnnotationType::TArrayBegin;
    } else if (s == "array-section-end") {
      return AnnotationType::TArrayEnd;
    } else if (s == "elt") {
      return AnnotationType::TElt;
    } else if (s == "elt-rep") {
      return AnnotationType::TEltRep;
    } else if (s == "elt-rep-end") {
      return AnnotationType::TEltRepEnd;
    } else if (s == "field-begin") {
      return AnnotationType::TFieldBegin;
    } else if (s == "field-name-end") {
      return AnnotationType::TFieldNameEnd;
    } else if (s == "field-value") {
      return AnnotationType::TFieldValue;
    } else if (s == "field-end") {
      return AnnotationType::TFieldEnd;
    } else if (s == "value-history-value") {
      return AnnotationType::TValueHistoryValue;
    } else if (s == "value-history-begin") {
      return AnnotationType::TValueHistoryBegin;
    } else if (s == "value-history-end") {
      return AnnotationType::TValueHistoryEnd;
    } else if (s == "signal") {
      return AnnotationType::TSignal;
    } else if (s == "signal-name") {
      return AnnotationType::TSignalName;
    } else if (s == "signal-name-end") {
      return AnnotationType::TSignalNameEnd;
    } else if (s == "signal-string") {
      return AnnotationType::TSignalString;
    } else if (s == "signal-string-end") {
      return AnnotationType::TSignalStringEnd;
    } else if (mIndex >= mOutput.length()) {
      return AnnotationType::TEOF;
    } else {
      return AnnotationType::TUnknown;;
    }
}

AnnotationType DebugReader::getLastAnnotation(const QByteArray &text)
{
    int curpos = text.length()-1;
    // Walk back until end of #26's
    while ((curpos >= 0) && (text[curpos] != 26))
        curpos--;

    curpos++;

    // Tiny rewrite of GetNextWord for special purposes
    QString s = "";
    while ((curpos < text.length()) && (text[curpos]>32)) {
        s = s + text[curpos];
        curpos++;
    }

    return getAnnotation(s);
}

AnnotationType DebugReader::getNextAnnotation()
{
    // Skip until end of #26's, i.e. GDB formatted output
    skipToAnnotation();

    // Get part this line, after #26#26
    return getAnnotation(getNextWord());
}

QString DebugReader::getNextFilledLine()
{
    // Walk up to an enter sequence
    while (mIndex<mOutput.length() && mOutput[mIndex]!=13 && mOutput[mIndex]!=10 && mOutput[mIndex]!=0)
        mIndex++;
    // Skip enter sequences (CRLF, CR, LF, etc.)
    while (mIndex<mOutput.length() && mOutput[mIndex]==13 && mOutput[mIndex]==10 && mOutput[mIndex]==0)
        mIndex++;
    // Return next line
    return getRemainingLine();
}

QString DebugReader::getNextLine()
{
    // Walk up to an enter sequence
    while (mIndex<mOutput.length() && mOutput[mIndex]!=13 && mOutput[mIndex]!=10 && mOutput[mIndex]!=0)
        mIndex++;

    // End of output. Exit
    if (mIndex>=mOutput.length())
        return "";
    // Skip ONE enter sequence (CRLF, CR, LF, etc.)
    if ((mOutput[mIndex] == 13) && (mOutput[mIndex] == 10)) // DOS
        mIndex+=2;
    else if (mOutput[mIndex] == 13)  // UNIX
        mIndex++;
    else if (mOutput[mIndex] == 10) // MAC
        mIndex++;
    // Return next line
    return getRemainingLine();
}

QString DebugReader::getNextWord()
{
    QString Result;

    // Called when at a space? Skip over
    skipSpaces();

    // Skip until a space
    while (mIndex<mOutput.length() && mOutput[mIndex]>32) {
        Result += mOutput[mIndex];
        mIndex++;
    }
    return Result;
}

QString DebugReader::getRemainingLine()
{
    QString Result;

    // Return part of line still ahead of us
    while (mIndex<mOutput.length() && mOutput[mIndex]!=13 && mOutput[mIndex]!=10 && mOutput[mIndex]!=0) {
        Result += mOutput[mIndex];
        mIndex++;
    }
    return Result;
}

void DebugReader::handleDisassembly()
{
    if (mDisassembly.isEmpty())
        return;

    // Get info message
    QString s = getNextLine();

    // the full function name will be saved at index 0
    mDisassembly.append(s.mid(36));

    s = getNextLine();

    // Add lines of disassembly
    while (!s.isEmpty() && (s != "End of assembler dump")) {
        mDisassembly.append(s);
        s = getNextLine();
    }

    dodisassemblerready = true;
}

void DebugReader::handleDisplay()
{
    QString s = getNextLine(); // watch index

    if (!findAnnotation(AnnotationType::TDisplayExpression))
        return;
    QString watchName = getNextLine(); // watch name

    // Find parent we're talking about
    auto result = mWatchVarList.find(watchName);
    if (result != mWatchVarList.end()) {
        PWatchVar watchVar = result.value();
        // Advance up to the value
        if (!findAnnotation(AnnotationType::TDisplayExpression))
            return;;
        // Refresh GDB index so we can undisplay this by index
        watchVar->gdbIndex = s.toInt();
        processWatchOutput(watchVar);
    }
}

void DebugReader::handleError()
{
    QString s = getNextLine(); // error text
    if (s.startsWith("Cannot find bounds of current function")) {
      //We have exited
      handleExit();
    } else if (s.startsWith("No symbol \"")) {
        int head = s.indexOf('\"');
        int tail = s.lastIndexOf('\"');
        QString watchName = s.mid(head+1, tail-head-1);

        // Update current...
        auto result = mWatchVarList.find(watchName);
        if (result != mWatchVarList.end()) {
            PWatchVar watchVar = result.value();
            //todo: update watch value to invalid
            mDebugger->invalidateWatchVar(watchVar);
            watchVar->gdbIndex = -1;
            dorescanwatches = true;
        }
    }
}

void DebugReader::handleExit()
{
    doprocessexited=true;
}

void DebugReader::handleFrames()
{
    QString s = getNextLine();

    // Is this a backtrace dump?
    if (s.startsWith("#")) {
        // Find function name
        if (!findAnnotation(AnnotationType::TFrameFunctionName))
            return;

        PTrace trace = std::make_shared<Trace>();
        trace->funcname = getNextLine();

        // Find argument list start
        if (!findAnnotation(AnnotationType::TFrameArgs))
            return;

        // Arguments are either () or detailed list
        s = getNextLine();

        while (peekNextAnnotation() == AnnotationType::TArgBegin) {

            // argument name
            if (!findAnnotation(AnnotationType::TArgBegin))
                return;

            s = s + getNextLine();

            // =
            if (!findAnnotation(AnnotationType::TArgNameEnd))
                return;
            s = s + ' ' + getNextLine() + ' '; // should be =

            // argument value
            if (!findAnnotation(AnnotationType::TArgValue))
                return;

            s = s + getNextLine();

            // argument end
            if (!findAnnotation(AnnotationType::TArgEnd))
                return;

            s = s + getNextLine();
        }

        trace->funcname = trace->funcname + s.trimmed();

        // source info
        if (peekNextAnnotation() == AnnotationType::TFrameSourceBegin) {
            // Find filename
            if (!findAnnotation(AnnotationType::TFrameSourceFile))
                return;
            trace->filename = getNextLine();
            // find line
            if (!findAnnotation(AnnotationType::TFrameSourceLine))
                return;
            trace->line = getNextLine().trimmed().toInt();
        } else {
            trace->filename = "";
            trace->line = 0;
        }
        mDebugger->backtraceModel()->addTrace(trace);

        // Skip over the remaining frame part...
        if (!findAnnotation(AnnotationType::TFrameEnd))
            return;

        // Not another one coming? Done!
        if (peekNextAnnotation() != AnnotationType::TFrameBegin) {
            // End of stack trace dump!
              dobacktraceready = true;
        }
    } else
        doupdatecpuwindow = true;
}

void DebugReader::handleLocalOutput()
{
    // name(spaces)hexvalue(tab)decimalvalue
    QString s = getNextFilledLine();

    bool breakLine = false;
    while (true) {
        if (s.startsWith("\032\032")) {
            s = TrimLeft(s);
            if (s == "No locals.") {
                return;
            }
            if (s == "No arguments.") {
                return;
            }
            //todo: update local view
//            if (breakLine and (MainForm.txtLocals.Lines.Count>0) then begin
//          MainForm.txtLocals.Lines[MainForm.txtLocals.Lines.Count-1] := MainForm.txtLocals.Lines[MainForm.txtLocals.Lines.Count-1] + s;
//        end else begin
//          MainForm.txtLocals.Lines.Add(s);
//        end;
            breakLine=false;
        } else {
            breakLine = true;
        }
        s = getNextLine();
        if (!breakLine && s.isEmpty())
            break;
    }
}

void DebugReader::handleLocals()
{
    //todo: clear local view
    handleLocalOutput();
}

void DebugReader::handleParams(){
    handleLocalOutput();
}

void DebugReader::handleRegisters()
{
    // name(spaces)hexvalue(tab)decimalvalue
    QString s = getNextFilledLine();

    while (true) {
        PRegister reg = std::make_shared<Register>();
        // Cut name from 1 to first space
        int x = s.indexOf(' ');
        reg->name = s.mid(0,x-1);
        s.remove(0,x);
        // Remove spaces
        s = TrimLeft(s);

        // Cut hex value from 1 to first tab
        x = s.indexOf('\t');
        if (x<0)
            x = s.indexOf(' ');
        reg->hexValue = s.mid(0,x - 1);
        s.remove(0,x); // delete tab too
        s = TrimLeft(s);

        // Remaining part contains decimal value
        reg->decValue = s;

        mRegisters.append(reg);
        s = getNextLine();
        if (s.isEmpty())
            break;
    }

    doregistersready = true;
}

void DebugReader::handleSignal()
{
    mSignal = getNextFilledLine(); // Program received signal

    if (!findAnnotation(AnnotationType::TSignalName))
        return;

    mSignal = mSignal + getNextFilledLine(); // signal code

    if (!findAnnotation(AnnotationType::TSignalNameEnd))
        return;

    mSignal = mSignal + getNextFilledLine(); // comma

    if (!findAnnotation(AnnotationType::TSignalString))
        return;

    mSignal = mSignal + getNextFilledLine(); // user friendly description

    if (!findAnnotation(AnnotationType::TSignalStringEnd))
        return;

    mSignal = mSignal + getNextFilledLine(); // period

    doreceivedsignal = true;
}

void DebugReader::handleSource()
{
    // source filename:line:offset:beg/middle/end:addr
    QString s = TrimLeft(getRemainingLine());

    // remove offset, beg/middle/end, address
    for (int i=0;i<3;i++) {
        int delimPos = s.lastIndexOf(':');
        if (delimPos >= 0)
            s.remove(delimPos,INT_MAX);
        else
            return; // Wrong format. Don't bother to continue
    }

    // get line
    int delimPos = s.lastIndexOf(':');
    if (delimPos >= 0) {
        mBreakPointLine = s.mid(delimPos+1).toInt();
        s.remove(delimPos, INT_MAX);
    }

    // get file
    mBreakPointFile = s;

    doupdateexecution = true;
    doupdatecpuwindow = true;
}

void DebugReader::handleValueHistoryValue()
{
    mEvalValue = processEvalOutput();
    doevalready = true;
}

AnnotationType DebugReader::peekNextAnnotation()
{
    int indexBackup = mIndex; // do NOT modifiy curpos
    AnnotationType result = getNextAnnotation();
    mIndex = indexBackup;
    return result;
}

void DebugReader::processDebugOutput()
{
    // Only update once per update at most
    //WatchView.Items.BeginUpdate;

    if (mInvalidateAllVars) {
         //invalidate all vars when there's first output
         invalidateAllVars();
         mInvalidateAllVars = false;
    }

    emit parseStarted();

   //try

   dobacktraceready = false;
   dodisassemblerready = false;
   doregistersready = false;
   dorescanwatches = false;
   doevalready = false;
   doprocessexited = false;
   doupdateexecution = false;
   doreceivedsignal = false;
   doupdatecpuwindow = false;
   doreceivedsfwarning = false;

   // Global checks
   if (mOutput.indexOf("warning: Source file is more recent than executable.") >= 0)
       doreceivedsfwarning = true;

   mIndex = 0;
   AnnotationType nextAnnotation;
   do {
       nextAnnotation = getNextAnnotation();
       switch(nextAnnotation) {
       case AnnotationType::TValueHistoryValue:
           handleValueHistoryValue();
           break;
       case AnnotationType::TSignal:
           handleSignal();
           break;
       case AnnotationType::TExit:
           handleExit();
           break;
       case AnnotationType::TFrameBegin:
           handleFrames();
           break;
       case AnnotationType::TInfoAsm:
           handleDisassembly();
           break;
       case AnnotationType::TInfoReg:
           handleRegisters();
           break;
       case AnnotationType::TLocal:
           handleLocals();
           break;
       case AnnotationType::TParam:
           handleParams();
           break;
       case AnnotationType::TErrorBegin:
           handleError();
           break;
       case AnnotationType::TDisplayBegin:
           handleDisplay();
           break;
       case AnnotationType::TSource:
           handleSource();
           break;
       }
   } while (nextAnnotation != AnnotationType::TEOF);

     // Only update once per update at most
   //finally
     //WatchView.Items.EndUpdate;
   //end;

   emit parseFinished();
}

QString DebugReader::processEvalOutput()
{
    int indent = 0;

    // First line gets special treatment
    QString result = getNextLine();
    if (result.startsWith('{'))
        indent+=4;

    // Collect all data, add formatting in between
    AnnotationType nextAnnotation;
    QString nextLine;
    bool shouldExit = false;
    do {
        nextAnnotation = getNextAnnotation();
        nextLine = getNextLine();
        switch(nextAnnotation) {
        // Change indent if { or } is found
        case AnnotationType::TFieldBegin:
            result += "\r\n" + QString(4,' ');
            break;
        case AnnotationType::TFieldValue:
            if (nextLine.startsWith('{') && (peekNextAnnotation() !=
                                             AnnotationType::TArrayBegin))
                indent+=4;
            break;
        case AnnotationType::TFieldEnd:
            if (nextLine.endsWith('}')) {
                indent-=4;
                result += "\r\n" + QString(4,' ');
            }
            break;
        case AnnotationType::TEOF:
        case AnnotationType::TValueHistoryEnd:
        case AnnotationType::TDisplayEnd:
            shouldExit = true;
        }
        result += nextLine;
    } while (!shouldExit);
}

void DebugReader::processWatchOutput(PWatchVar WatchVar)
{
    //todo
}

void DebugReader::runNextCmd()
{
    bool doUpdate=false;

    auto action = finally([this,&doUpdate] {
        if (doUpdate) {
            emit updateWatch();
        }
    });
    QMutexLocker locker(&mCmdQueueMutex);
    if (mCmdQueue.isEmpty()) {
        if ((mCurrentCmd) && (mCurrentCmd->updateWatch)) {
            doUpdate=true;
            if (mUpdateCount>0) {
                mUpdateCount=0;
            }
        }
        return;
    }

    if (mCurrentCmd) {
        mCurrentCmd.reset();
    }

    PDebugCommand pCmd = mCmdQueue.dequeue();
    mCmdRunning = true;
    mCurrentCmd = pCmd;

    QByteArray s;
    s=pCmd->command.toLocal8Bit();
    if (!pCmd->params.isEmpty()) {
        s+=' '+pCmd->params.toLocal8Bit();
    }
    s+= "\n";
    if (mProcess->write(s)<0) {
        emit writeToDebugFailed();
    }

//  if devDebugger.ShowCommandLog or pCmd^.ShowInConsole then begin
    if (pSettings->debugger().showCommandLog() || pCmd->showInConsole) {
        //update debug console
        // if not devDebugger.ShowAnnotations then begin
        if (!pSettings->debugger().showAnnotations()) {
//            if MainForm.DebugOutput.Lines.Count>0 then begin
//              MainForm.DebugOutput.Lines.Delete(MainForm.DebugOutput.Lines.Count-1);
//            end;
            emit changeDebugConsoleLastLine("(gdb)"+pCmd->command + ' ' + pCmd->params);
//            MainForm.DebugOutput.Lines.Add('(gdb)'+pCmd^.Cmd + ' ' + pCmd^.params);
//            MainForm.DebugOutput.Lines.Add('');
        } else {
            emit changeDebugConsoleLastLine("(gdb)"+pCmd->command + ' ' + pCmd->params);
//            MainForm.DebugOutput.Lines.Add(pCmd^.Cmd + ' ' + pCmd^.params);
//            MainForm.DebugOutput.Lines.Add('');
        }
    }
}

void DebugReader::skipSpaces()
{
    while (mIndex < mOutput.length() &&
           (mOutput[mIndex]=='\t' || mOutput[mIndex]==' '))
        mIndex++;
}

void DebugReader::skipToAnnotation()
{
    // Walk up to the next annotation
    while (mIndex < mOutput.length() &&
           (mOutput[mIndex]!=26))
        mIndex++;
    // Crawl through the remaining ->'s
    while (mIndex < mOutput.length() &&
           (mOutput[mIndex]==26))
        mIndex++;
}

QString DebugReader::debuggerPath() const
{
    return mDebuggerPath;
}

void DebugReader::setDebuggerPath(const QString &debuggerPath)
{
    mDebuggerPath = debuggerPath;
}

void DebugReader::stopDebug()
{
    mStop = true;
}


void DebugReader::run()
{
    mStop = false;
    bool errorOccurred = false;
    QString cmd = mDebuggerPath;
    QString arguments = "--annotate=2 --silent";
    QString workingDir = QFileInfo(mDebuggerPath).path();

    mProcess = new QProcess();
    mProcess->setProgram(cmd);
    mProcess->setArguments(QProcess::splitCommand(arguments));
    mProcess->setWorkingDirectory(workingDir);

    connect(mProcess, &QProcess::errorOccurred,
                    [&](){
                        errorOccurred= true;
                    });
//    mProcess.connect(&process, &QProcess::readyReadStandardError,[&process,this](){
//        this->error(QString::fromLocal8Bit( process.readAllStandardError()));
//    });
//    mProcess.connect(&mProcess, &QProcess::readyReadStandardOutput,[&process,this](){
//        this->log(QString::fromLocal8Bit( process.readAllStandardOutput()));
//    });
//    process.connect(&mProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),[&process,this](){
//        this->error(COMPILE_PROCESS_END);
//    });
    mProcess->start();
    mProcess->waitForStarted(5000);
    mStartSemaphore.release(1);
    QByteArray buffer;
    QByteArray readed;
    while (true) {
        mProcess->waitForFinished(100);
        if (mProcess->state()!=QProcess::Running) {
            break;
        }
        if (mStop) {
            mProcess->terminate();
            break;
        }
        if (errorOccurred)
            break;
        readed = mProcess->readAll();
        buffer += readed;
        if (getLastAnnotation(buffer) == AnnotationType::TPrompt) {
            mOutput = buffer;
            processDebugOutput();
            buffer.clear();
            mCmdRunning = false;
            runNextCmd();
        } else if (!mCmdRunning && readed.isEmpty()){
            runNextCmd();
        }
    }
    if (errorOccurred) {
        emit processError(mProcess->error());
    }
}



BreakpointModel::BreakpointModel(QObject *parent):QAbstractTableModel(parent)
{

}

int BreakpointModel::rowCount(const QModelIndex &) const
{
    return mList.size();
}

int BreakpointModel::columnCount(const QModelIndex &) const
{
    return 3;
}

QVariant BreakpointModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()<0 || index.row() >= static_cast<int>(mList.size()))
        return QVariant();
    PBreakpoint breakpoint = mList[index.row()];
    if (!breakpoint)
        return QVariant();
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return breakpoint->filename;
        case 1:
            if (breakpoint->line>0)
                return breakpoint->line;
            else
                return "";
        case 2:
            return breakpoint->condition;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QVariant BreakpointModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role ==  Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Filename");
        case 1:
            return tr("Line");
        case 2:
            return tr("Condition");
        }
    }
    return QVariant();
}

void BreakpointModel::addBreakpoint(PBreakpoint p)
{
    beginInsertRows(QModelIndex(),mList.size(),mList.size());
    mList.push_back(p);
    endInsertRows();
}

void BreakpointModel::clear()
{
    beginRemoveRows(QModelIndex(),0,mList.size()-1);
    mList.clear();
    endRemoveRows();
}

void BreakpointModel::removeBreakpoint(int row)
{
    beginRemoveRows(QModelIndex(),row,row);
    mList.removeAt(row);
    endRemoveRows();
}

PBreakpoint BreakpointModel::setBreakPointCondition(int index, const QString &condition)
{
    beginResetModel();
    PBreakpoint breakpoint = mList[index];
    breakpoint->condition = condition;
    endResetModel();
    return breakpoint;
}

const QList<PBreakpoint> &BreakpointModel::breakpoints() const
{
    return mList;
}


BacktraceModel::BacktraceModel(QObject *parent):QAbstractTableModel(parent)
{

}

int BacktraceModel::rowCount(const QModelIndex &) const
{
    return mList.size();
}

int BacktraceModel::columnCount(const QModelIndex &) const
{
    return 3;
}

QVariant BacktraceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()<0 || index.row() >= static_cast<int>(mList.size()))
        return QVariant();
    PTrace trace = mList[index.row()];
    if (!trace)
        return QVariant();
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return trace->funcname;
        case 1:
            return trace->filename;
        case 2:
            if (trace->line>0)
                return trace->line;
            else
                return "";
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QVariant BacktraceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role ==  Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Function");
        case 1:
            return tr("Filename");
        case 2:
            return tr("Line");
        }
    }
    return QVariant();
}

void BacktraceModel::addTrace(PTrace p)
{
    beginInsertRows(QModelIndex(),mList.size(),mList.size());
    mList.push_back(p);
    endInsertRows();
}

void BacktraceModel::clear()
{
    beginRemoveRows(QModelIndex(),0,mList.size()-1);
    mList.clear();
    endRemoveRows();
}

void BacktraceModel::removeTrace(int row)
{
    beginRemoveRows(QModelIndex(),row,row);
    mList.removeAt(row);
    endRemoveRows();
}

const QList<PTrace> &BacktraceModel::backtraces() const
{
    return mList;
}

QModelIndex WatchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row,column,parent))
        return QModelIndex();

    WatchVar* parentItem;
    PWatchVar pChild;
    if (!parent.isValid()) {
        parentItem = nullptr;
        pChild = mWatchVars[row];
    } else {
        parentItem = static_cast<WatchVar*>(parent.internalPointer());
        pChild = parentItem->children[row];
    }
    if (pChild)
        return createIndex(row,column,pChild.get());
    return QModelIndex();
}

static int getWatchIndex(WatchVar* var, const QList<PWatchVar> list) {
    for (int i=0;i<list.size();i++) {
        PWatchVar v = list[i];
        if (v.get() == var) {
            return i;
        }
    }
}

QModelIndex WatchModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    WatchVar* childItem = static_cast<WatchVar*>(index.internalPointer());
    WatchVar* parentItem =childItem->parent;

    //parent is root
    if (parentItem == nullptr) {
        return QModelIndex();
    }
    int row;
    WatchVar* grandItem = parentItem->parent;
    if (grandItem == nullptr) {
        row = getWatchIndex(parentItem,mWatchVars);
    } else {
        row = getWatchIndex(parentItem,grandItem->children);
    }
    return createIndex(row,0,parentItem);
}

int WatchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return mWatchVars.count();
    } else {
        WatchVar* parentItem = static_cast<WatchVar*>(parent.internalPointer());
        return parentItem->children.count();
    }
}

int WatchModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

void WatchModel::addWatchVar(PWatchVar watchVar)
{
    for (PWatchVar var:mWatchVars) {
        if (watchVar->name == var->name) {
            return;
        }
    }
    mWatchVars.append(watchVar);
}

void WatchModel::removeWatchVar(const QString &name)
{
    for (PWatchVar var:mWatchVars) {
        if (name == var->name) {
            mWatchVars.removeOne(var);
        }
    }
}

void WatchModel::removeWatchVar(int gdbIndex)
{
    for (PWatchVar var:mWatchVars) {
        if (gdbIndex == var->gdbIndex) {
            mWatchVars.removeOne(var);
        }
    }
}

void WatchModel::clear()
{
    mWatchVars.clear();
}

QList<PWatchVar> &WatchModel::watchVars()
{
    return mWatchVars;
}

PWatchVar WatchModel::findWatchVar(const QString &name)
{
    for (PWatchVar var:mWatchVars) {
        if (name == var->name) {
            return var;
        }
    }
}

PWatchVar WatchModel::findWatchVar(int gdbIndex)
{
    for (PWatchVar var:mWatchVars) {
        if (gdbIndex == var->gdbIndex) {
            return var;
        }
    }
}
