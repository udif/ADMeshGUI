// (c) 2015 David Vyvlečka, AGPLv3

#include "admeshcontroller.h"
#include <QFileDialog>
#include <QtWidgets>
#include <string>
#include <math.h>

using namespace std;

admeshController::admeshController(QObject *parent) :
    QObject(parent)
{
    count = 0;
    mode = 0;
    versor[0] = 1.0;
    versor[1] = 1.0;
    versor[2] = 1.0;
    useVersor = true;
    rot = 0.0;
    x_translate = 0.0;
    y_translate = 0.0;
    z_translate = 0.0;
    a_cut=0;
    b_cut=0;
    c_cut=1;
    d_cut=0;
    planeMax=1;
    rel_translate = true;
    fixall_flag = true;
    exact_flag = false;
    tolerance_flag = false;
    tolerance = 0.0;
    increment_flag = false;
    increment = 0.0;
    nearby_flag = false;
    iterations = 1;
    remove_unconnected_flag = false;
    fill_holes_flag = false;
    normal_directions_flag = false;
    normal_values_flag = false;
    QPixmap pixmap(32,32);
    pixmap.fill(QColor(Qt::transparent));
    visibleIcon = QIcon(pixmap);
    hiddenIcon = QIcon("://Resources/hide.svg");
    QSettings settings;
    history.setLimitSize(settings.value("sizeLimit", HISTORY_LIMIT).toInt());
    openPath = settings.value("openPath", QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0)).value<QString>();
    setDrawColor(settings.value("color",QColor(Qt::green)).value<QColor>(), settings.value("badColor",QColor(Qt::red)).value<QColor>());
    planeShown=false;
}

admeshController::~admeshController()
{    
    listModel->clear();
    delete listModel;
    listView->close();
}

void admeshController::setMode(int m)
{
    mode = m;
}

void admeshController::allowFunctions()
{
    if(history.hasUndos())allowUndo(true);
    else allowUndo(false);
    if(history.hasRedos())allowRedo(true);
    else allowRedo(false);
}

void admeshController::allowSelectionFunctions()
{
    if(selectedCount()==1){
        allowSave(true);
        allowSaveAs(true);
        allowExport(true);
        allowClose(true);
    }else if(selectedCount()>1){
        allowSave(true);
        allowSaveAs(false);
        allowExport(false);
        allowClose(true);
    }else{
        allowSave(false);
        allowSaveAs(false);
        allowExport(false);
        allowClose(false);
    }
}

void admeshController::pushHistory()
{
    unsigned long size = 0;
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(!objectList[i]->hasReferences()){    //inactive object is reused via same pointer
            size += objectList[i]->getSize();
        }
    }
    history.add(objectList, size);
    allowFunctions();
    allowSelectionFunctions();
}

void admeshController::renewListView()
{
    listModel->clear();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){        
            addItemToView(objectList[i]);
    }
}

void admeshController::renewList()
{
    QList <MeshObject*> tmp = objectList;
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(!tmp[i]->isActive()){    //inactive object is reused via same pointer
            tmp[i]->addReference();
        }else{
            tmp[i] = new MeshObject(*tmp[i]);
        }
    }
    objectList = tmp;
}

void admeshController::undo()
{
    objectList = history.undo();
    count = objectList.size();
    renewListView();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        objectList[i]->updateGeometry();
    }
    if(selectedCount()>0)statusBar->setText(_("Status: Returned 1 step back in history."));
    reDrawSignal();
    allowFunctions();
    allowSelectionFunctions();
    if(count == 0) enableEdit(false);
    else enableEdit(true);
}

void admeshController::redo()
{
    objectList = history.redo();
    count = objectList.size();
    renewListView();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        objectList[i]->updateGeometry();
    }
    if(selectedCount()>0)statusBar->setText(_("Status: Returned 1 step forward in history."));
    reDrawSignal();
    allowFunctions();
    allowSelectionFunctions();
    if(count == 0) enableEdit(false);
    else enableEdit(true);
}

void admeshController::drawAll(QGLShaderProgram *program)
{

    program->setUniformValue("plane", false);
    if(mode == 0){
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        program->setUniformValue("differ_hue", true);
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            if(objectList[i]->isActive()){
                //if(objectList[i]->getName()=="plane")
                if(objectList[i]->isPlane())
                {

                    program->setUniformValue("plane", true);
                    glEnable(GL_BLEND);
                    objectList[i]->drawGeometry(program);
                    glDisable(GL_BLEND);
                    program->setUniformValue("plane", false);
                    continue;
                }
                program->setUniformValue("color", color);
                program->setUniformValue("badColor", badColor);
            }else if(!objectList[i]->isHidden()){
                program->setUniformValue("color", GREY);
                program->setUniformValue("badColor", GREY);
            }else{
                continue;
            }
            objectList[i]->drawGeometry(program);
        }
        program->setUniformValue("differ_hue", false);
    }else if(mode == 1){
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            if(objectList[i]->isActive()){
                program->setUniformValue("color", BLACK);
                program->setUniformValue("badColor", BLACK);
            }else if(!objectList[i]->isHidden()){
                program->setUniformValue("color", GREY);
                program->setUniformValue("badColor", GREY);
            }else{
                continue;
            }
            objectList[i]->drawGeometry(program);
        }
    }else if(mode == 2){
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){

            if(objectList[i]->isActive()){
                program->setUniformValue("color", BLACK);
                program->setUniformValue("badColor", BLACK);
            }else if(!objectList[i]->isHidden()){
                program->setUniformValue("color", GREY);
                program->setUniformValue("badColor", GREY);
            }else{
                continue;
            }
            objectList[i]->drawGeometry(program);
        }
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        glEnable( GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( 1, 1 );
        program->setUniformValue("differ_hue", true);
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            if(objectList[i]->isActive()){
                //if(objectList[i]->getName()=="plane")
                if(objectList[i]->isPlane())
                {
                    program->setUniformValue("plane", true);
                    glEnable(GL_BLEND);
                    objectList[i]->drawGeometry(program);
                    glDisable(GL_BLEND);
                    program->setUniformValue("plane", false);
                    continue;
                }
                program->setUniformValue("color", color);
                program->setUniformValue("badColor", badColor);
            }else if(!objectList[i]->isHidden()){
                program->setUniformValue("color", GREY);
                program->setUniformValue("badColor", GREY);
            }else{
                continue;
            }
            objectList[i]->drawGeometry(program);
        }
        program->setUniformValue("differ_hue", false);
        glDisable( GL_POLYGON_OFFSET_FILL );
    }
}

void admeshController::drawPicking(QGLShaderProgram *program)
{
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(!objectList[i]->isHidden()){
            int index = (int)i;
            float b = (float)(index%255)/255;
            float g = (float)((index/255)%255)/255;
            float r = (float)((index/(255*255))%255)/255;
            program->setUniformValue("color", QVector3D(r,g,b));
            objectList[i]->drawGeometry(program);
        }
    }
}

int admeshController::selectedCount()
{
    int selected = 0;
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive()) selected++;
    }
    return selected;
}

void admeshController::setActiveByIndex(GLuint id)
{    
    if((id<ITEMS_LIMIT && count > 1) || (id<ITEMS_LIMIT && count == 1 && !objectList[id]->isActive())) {
        QStandardItem *item = listModel->item(id);
        listView->selectionModel()->select( listModel->indexFromItem(item), QItemSelectionModel::Toggle );
    }
    reDrawSignal();
    allowSelectionFunctions();
}

void admeshController::setAllActive()
{
    listView->selectAll();
    reDrawSignal();
    allowSelectionFunctions();
}

void admeshController::setAllInactive()
{
    if(count > 1){
        listView->clearSelection();
    }    
    reDrawSignal();
    allowSelectionFunctions();
}

void admeshController::setAllInverseActive()
{
    if(count > 1){
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            QStandardItem *item = listModel->item(i);
            listView->selectionModel()->select( listModel->indexFromItem(item), QItemSelectionModel::Toggle );
        }
    }
    reDrawSignal();
    allowSelectionFunctions();
}

void admeshController::hide()
{
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isSelected()){
            objectList[i]->setHidden();
            objectList[i]->toggleSelected();
            QStandardItem *Item = listModel->item(i);
            Item->setData(hiddenIcon, Qt::DecorationRole);
        }
    }
    listView->clearSelection();
    reDrawSignal();
}

void admeshController::unhide()
{
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isSelected()){
            objectList[i]->setVisible();
            QStandardItem *Item = listModel->item(i);
            Item->setData(visibleIcon, Qt::DecorationRole);
        }
    }
    reDrawSignal();
    allowFunctions();
    allowSelectionFunctions();
}

void admeshController::unhideAll()
{
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        objectList[i]->setVisible();
    }
    reDrawSignal();
}

void admeshController::setDrawColor(QColor col, QColor badCol){
    color = QVector3D(col.redF(),col.greenF(),col.blueF());
    badColor = QVector3D(badCol.redF(),badCol.greenF(),badCol.blueF());
}

void admeshController::setHistoryLimit(int lim)
{
    history.setLimitSize(lim);
}

void admeshController::addUIItems(QLabel *l,QListView *v)
{
    statusBar = l;
    listView = v;
    delete listView->model();
    listModel = new QStandardItemModel();

    listView->setModel(listModel);
    connect(listView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(handleSelectionChanged(QItemSelection, QItemSelection)));
}

void admeshController::handleSelectionChanged(QItemSelection selection, QItemSelection deselection)
{
    foreach(QModelIndex index, selection.indexes()){
        if(index.row() < count) objectList[index.row()]->setSelected();
    }
    foreach(QModelIndex index, deselection.indexes()){
        if(index.row() < count) objectList[index.row()]->setDeselected();
    }
    reDrawSignal();
    allowSelectionFunctions();
}

void admeshController::addItemToView(MeshObject* item){
    QFileInfo fi=item->getName();
    QStandardItem *Item = new QStandardItem(fi.fileName());
    if(item->isHidden())Item->setData(hiddenIcon, Qt::DecorationRole);
    else Item->setData(visibleIcon, Qt::DecorationRole);
    listModel->appendRow(Item);
    QModelIndex index = listModel->indexFromItem(Item);
    if(item->isActive()) listView->selectionModel()->select( index, QItemSelectionModel::Select );
}

QString admeshController::getInfo()
{
    QString text = "<table>";
    float minx, miny, minz, maxx, maxy, maxz, sizex, sizey, sizez, num_facets, deg_facets, edges_fixed, facets_removed, facet_sadded, facets_reversed, backward, normals_fixed, volume;
    minx = miny = minz = maxx = maxy = maxz = sizex = sizey = sizez = num_facets = deg_facets = edges_fixed = facets_removed = facet_sadded = facets_reversed = backward = normals_fixed = volume = 0;
    int objects = 0;
    bool initialized = false;
    float* arr;
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive() && !objectList[i]->isPlane())//objectList[i]->getName()!="plane"){
        {
            arr = objectList[i]->getInfo();
            if(!initialized){
                minx = arr[0];
                miny = arr[1];
                minz = arr[2];
                maxx = arr[3];
                maxy = arr[4];
                maxz = arr[5];
                num_facets = arr[6];
                deg_facets = arr[7];
                edges_fixed = arr[8];
                facets_removed = arr[9];
                facet_sadded = arr[10];
                facets_reversed = arr[11];
                backward = arr[12];
                normals_fixed = arr[13];
                volume = arr[14];
                initialized = true;
            }else{
                if(arr[0]<minx)minx = arr[0];
                if(arr[1]<miny)miny = arr[1];
                if(arr[2]<minz)minz = arr[2];
                if(arr[3]>maxx)maxx = arr[3];
                if(arr[4]>maxy)maxy = arr[4];
                if(arr[5]>maxz)maxz = arr[5];               
                num_facets += arr[6];
                deg_facets += arr[7];
                edges_fixed += arr[8];
                facets_removed += arr[9];
                facet_sadded += arr[10];
                facets_reversed += arr[11];
                backward += arr[12];
                normals_fixed += arr[13];
                volume += arr[14];
            }
            objects++;
            delete []arr;
        }
        sizex = maxx - minx;
        sizey = maxy - miny;
        sizez = maxz - minz;
        planeMax=max(abs(maxx),abs(maxy))*1.41;
    }
    if(initialized){
        QTextStream(&text) << "<tr><td width=\"60%\" class=\"desc\">"<<_("Objects selected:")<<"</td><td width=\"40%\">"<<objects<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Min X:")<<"</td><td width=\"40%\">"<<minx<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Max X:")<<"</td><td width=\"40%\">"<<maxx<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Size X:")<<"</td><td width=\"40%\">"<<sizex<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Min Y:")<<"</td><td width=\"40%\">"<<miny<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Max Y:")<<"</td><td width=\"40%\">"<<maxy<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Size Y:")<<"</td><td width=\"40%\">"<<sizey<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Min Z:")<<"</td><td width=\"40%\">"<<minz<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Max Z:")<<"</td><td width=\"40%\">"<<maxz<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Size Z:")<<"</td><td width=\"40%\">"<<sizez<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Number of facets:")<<"</td><td width=\"40%\">"<<num_facets<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Degenerate facets:")<<"</td><td width=\"40%\">"<<deg_facets<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Edges fixed:")<<"</td><td width=\"40%\">"<<edges_fixed<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Facets removed:")<<"</td><td width=\"40%\">"<<facets_removed<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Facets added:")<<"</td><td width=\"40%\">"<<facet_sadded<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Facets reversed:")<<"</td><td width=\"40%\">"<<facets_reversed<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Backwards edges:")<<"</td><td width=\"40%\">"<<backward<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Normals fixed:") <<"</td><td width=\"40%\">"<<normals_fixed<<"</td></tr>" <<
                              "<tr><td width=\"60%\" class=\"desc\">"<<_("Total volume:") <<"</td><td width=\"40%\">"<<volume<<"</td></tr><tr></tr>";
    }
    return text;
}

void admeshController::openSTL()
{
    QStringList fileNameList = QFileDialog::getOpenFileNames((QWidget*)parent(), _("Open STL"), openPath, _("STL (*.stl *.STL)"));
    int opened = 0;
    renewList();
    QString fileName;
    for(QStringList::Iterator it = fileNameList.begin(); it != fileNameList.end(); it++){
        fileName = *it;
        if(!fileName.isEmpty()){
            MeshObject* tmp = new MeshObject;
            if(!tmp->loadGeometry(fileName)){
                QString msg;
                QTextStream(&msg) << _("File ") << fileName << _(" could not be opened.\n");
                QMessageBox::critical(NULL, _("Error"), msg);
                continue;
            }else{
                objectList.push_back(tmp);
                objectList.back()->setSelected();
                addItemToView(objectList.back());
                opened++;
            }
            count++;
        }
    }    
    pushHistory();
    if(opened>0){
        openPath = QFileInfo(fileName).path(); // save path used
        statusBar->setText(QString(ngettext("Status: %1 file opened", "Status: %1 files opened", opened)).arg(opened));
        reCalculatePosition();
        reDrawSignal();
        enableEdit(true);
    }
}


void admeshController::openSTLbyName(const char* filename)
{
    QString fileName(filename);
    MeshObject* tmp = new MeshObject;
    if(!tmp->loadGeometry(fileName)){
        QString msg;
        QTextStream(&msg) << _("File ") << fileName << _(" could not be opened.\n");
        QMessageBox::critical(NULL, _("Error"), msg);        
        return;
    }else{
        renewList();
        objectList.push_back(tmp);
        objectList.back()->setSelected();
        addItemToView(objectList.back());
        pushHistory();
    }
    count++;
    if(selectedCount()>0)statusBar->setText(QString(ngettext("Status: %1 file opened", "Status: %1 files opened", selectedCount())).arg(selectedCount()));
    reCalculatePosition();
    reDrawSignal();
    enableEdit(true);
    allowSelectionFunctions();
}

void admeshController::closeSTL()
{
    renewList();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isSelected()){
            delete objectList[i];
            objectList.erase(objectList.begin() + i);
            --count;
            --i;
        }
    }
    pushHistory();
    if(count == 1) objectList[0]->setSelected();
    else if(count == 0) enableEdit(false);
    renewListView();
    reDrawSignal();
    allowFunctions();
    allowSelectionFunctions();
}

void admeshController::saveAs()
{
    if(selectedCount()!=1){
        QString msg;
        QTextStream(&msg) << _("Select only one file to save.\n");
        QMessageBox::warning(NULL, _("Warning"), msg);
        return;
    }
    MeshObject *fileToSave = NULL;
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
         if(objectList[i]->isActive()) {
             fileToSave = objectList[i];
         }
    }
    QString filter="STL_ascii (*.stl)";
    QString fileName=QFileDialog::getSaveFileName((QWidget*)parent(), _("Save as"), fileToSave->getName(), _("STL_ascii (*.stl);;STL_binary (*.stl)"), &filter);
    if(!fileName.isEmpty()){
        fileName=fileName.section(".",0,0);
        fileName+=".stl";
        if(filter == "STL_ascii (*.stl)"){
            fileToSave->saveAs(fileName, 1);
            renewListView();
            statusBar->setText(_("Status: File saved as ASCII STL"));
        }else if(filter == "STL_binary (*.stl)"){
            fileToSave->saveAs(fileName, 2);
            renewListView();
            statusBar->setText(_("Status: File saved as binary STL"));
        }
    }
}

void admeshController::saveObject(MeshObject* object)
{
    if(!object->isSaved() && object->hasValidName()){ // current mesh is saveable
        object->save();
        statusBar->setText(_("Status: File saved"));
    }else if(!object->isSaved()){ // current mesh has no valid name stored (eg. merged one)
        QString filter="STL_ascii (*.stl)";
        QString fileName=QFileDialog::getSaveFileName((QWidget*)parent(), _("Save as"), object->getName(), _("STL_ascii (*.stl);;STL_binary (*.stl)"), &filter);
        if(!fileName.isEmpty()){
            fileName=fileName.section(".",0,0);
            fileName+=".stl";
            if(filter == "STL_ascii (*.stl)"){
               object->saveAs(fileName, 1);
               statusBar->setText(_("Status: File saved as ASCII STL"));
            }else if(filter == "STL_binary (*.stl)"){
               object->saveAs(fileName, 2);
               statusBar->setText(_("Status: File saved as binary STL"));
            }
        }
    }
}

void admeshController::save()
{
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive()){
             saveObject(objectList[i]);
        }
    }
}

bool admeshController::saveOnClose()
{
    QMessageBox msgBox((QWidget*)parent());
    msgBox.setText(_("File has been modified."));
    msgBox.setInformativeText(_("Do you want to save changes?"));
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel | QMessageBox::NoToAll);
    msgBox.setButtonText(QMessageBox::Save, _("Save"));
    msgBox.setButtonText(QMessageBox::Discard, _("Discard"));
    msgBox.setButtonText(QMessageBox::Cancel, _("Cancel"));
    msgBox.setButtonText(QMessageBox::NoToAll, _("Discard all"));
    msgBox.setDefaultButton(QMessageBox::Save);
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(!objectList[i]->isSaved()){
            QString msg;
            QTextStream(&msg) << _("File ") << objectList[i]->getName() << _(" has been modified.");
            msgBox.setText(msg);
            int ret = msgBox.exec();
            switch (ret) {
               case QMessageBox::NoToAll:
                   return true;
               case QMessageBox::Save:
                   saveObject(objectList[i]);
                   break;
               case QMessageBox::Cancel:
                   return false;
               default:
                   break;
             }
        }
    }
    return true;
}

void admeshController::exportSTL()
{
    if(selectedCount()!=1){
        QString msg;
        QTextStream(&msg) << _("Select only one file to export.\n");
        QMessageBox::warning(NULL, _("Warning"), msg);
        return;
    }
    MeshObject *fileToExport = NULL;
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
         if(objectList[i]->isActive()) {
             fileToExport = objectList[i];
         }
    }
    QString filter="OBJ (*.obj)";
    QString fileName=QFileDialog::getSaveFileName((QWidget*)parent(), _("Export as"), fileToExport->getName().section(".",0,0), _("OBJ (*.obj);;OFF (*.off);;DXF (*.dxf);;VRML (*.vrml)"), &filter);
    if(!fileName.isEmpty()){
        fileName=fileName.section(".",0,0);
        if(filter == "OBJ (*.obj)"){
            fileName+=".obj";
            fileToExport->exportSTL(fileName, 1);
            statusBar->setText(_("Status: File exported to OBJ format"));
        }else if(filter == "OFF (*.off)"){
            fileName+=".off";
            fileToExport->exportSTL(fileName, 2);
            statusBar->setText(_("Status: File exported to OFF format"));
        }else if(filter == "DXF (*.dxf)"){
            fileName+=".dxf";
            fileToExport->exportSTL(fileName, 3);
            statusBar->setText(_("Status: File exported to DXF format"));
        }else if(filter == "VRML (*.vrml)"){
            fileName+=".vrml";
            fileToExport->exportSTL(fileName, 4);
            statusBar->setText(_("Status: File exported to VRML format"));
        }
    }
}

void admeshController::writeSettings()
{
    QSettings settings;
    settings.setValue("rendermode", mode);
    settings.setValue("openPath", openPath);
}

float admeshController::getMaxDiameter()
{
    if(objectList.back()) return objectList.back()->getDiameter();
    else return 0.0;
}

void admeshController::setVersorX(double factor)
{
    if(useVersor){
        versor[0]=factor;
    }else{
        versor[0]=versor[1]=versor[2]=factor;
        scaleSignal(factor);
    }
}

void admeshController::setVersorY(double factor)
{
    if(useVersor){
        versor[1]=factor;
    }else{
        versor[0]=versor[1]=versor[2]=factor;
        scaleSignal(factor);
    }
}

void admeshController::setVersorZ(double factor)
{
    if(useVersor){
        versor[2]=factor;
    }else{
        versor[0]=versor[1]=versor[2]=factor;
        scaleSignal(factor);
    }
}

void admeshController::setVersor()
{
    useVersor = !useVersor;
    if(!useVersor)scaleSignal(versor[0]); //set all fields to X value
}

void admeshController::scale()
{
    if(versor[0] != 0.0 || versor[1] != 0.0 || versor[2] != 0.0){
        #ifdef QT_DEBUG
            start_time = clock();
        #endif
        bool deselected=deselectPlane();
        renewList();
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            if(objectList[i]->isActive())objectList[i]->scale(versor);
        }
        int scaled = selectedCount();
        if(scaled>0)statusBar->setText(QString(ngettext("Status: %1 mesh scaled", "Status: %1 meshes scaled", scaled)).arg(scaled));
        pushHistory();
        #ifdef QT_DEBUG
            cout << "Scale took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
        #endif
        setPlaneSelection(deselected);
    }

    reDrawSignal();
}

void admeshController::mirrorXY()
{
    #ifdef QT_DEBUG
        start_time = clock();
    #endif
    bool deselected=deselectPlane();
    renewList();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive())objectList[i]->mirrorXY();
    }
    int mirrored = selectedCount();
    if(mirrored>0)statusBar->setText(QString(ngettext("Status: %1 mesh mirrored along XY plane", "Status: %1 meshes mirrored along XY plane", mirrored)).arg(mirrored));
    pushHistory();
    #ifdef QT_DEBUG
        cout << "Mirror XY took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
    #endif
    setPlaneSelection(deselected);
    reDrawSignal();
}

void admeshController::mirrorYZ()
{
    #ifdef QT_DEBUG
        start_time = clock();
    #endif
    bool deselected=deselectPlane();
    renewList();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive())objectList[i]->mirrorYZ();
    }
    int mirrored = selectedCount();
    if(mirrored>0)statusBar->setText(QString(ngettext("Status: %1 mesh mirrored along YZ plane", "Status: %1 meshes mirrored along YZ plane", mirrored)).arg(mirrored));
    pushHistory();
    #ifdef QT_DEBUG
        cout << "Mirror YZ took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
    #endif
    setPlaneSelection(deselected);
    reDrawSignal();
}

void admeshController::mirrorXZ()
{
    #ifdef QT_DEBUG
        start_time = clock();
    #endif
    bool deselected=deselectPlane();
    renewList();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive())objectList[i]->mirrorXZ();
    }
    int mirrored = selectedCount();
    if(mirrored>0)statusBar->setText(QString(ngettext("Status: %1 mesh mirrored along XZ plane", "Status: %1 meshes mirrored along XZ plane", mirrored)).arg(mirrored));
    pushHistory();
    #ifdef QT_DEBUG
        cout << "Mirror XZ took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
    #endif
    setPlaneSelection(deselected);
    reDrawSignal();
}

void admeshController::setRot(double angle)
{
    rot = (float)angle;
}


void admeshController::resetPlane()
{
   stl_facet facet1;
   stl_facet facet2;
   stl_vertex tmp;
   float a=planeMax;
   tmp.x=a;
   tmp.y=-a;
   tmp.z=0.0;
   facet1.vertex[0]=tmp;
   facet2.vertex[0]=tmp;
   tmp.y=a;
   facet1.vertex[1]=tmp;
   tmp.x=-a;
   facet1.vertex[2]=tmp;
   facet2.vertex[1]=tmp;
   tmp.y=-a;
   facet2.vertex[2]=tmp;
   plane->facet_start[0] = facet1;
   stl_facet_stats(plane, facet1, 1);
   plane->facet_start[1] = facet2;
   stl_facet_stats(plane, facet2, 0);

}
void admeshController::showPlane()
{
    /*
    potrbeuju addnout nebo removnout plane
    potrebuju ji scalnout, aby byla velka podle modelu
    stl_stats.size vraci boundig box
    */

    //if(rel_translate) rel_translate = false;
    //else rel_translate = true;
if (!planeShown)
{

   //stl_file* plane=new stl_file;
   plane=new stl_file;
   plane->stats.number_of_facets=2;
   plane->stats.original_num_facets=0;
   plane->stats.type = inmemory;
   plane->v_indices = NULL;
   plane->v_shared = NULL;
   plane->neighbors_start = NULL;
   stl_clear_error(plane);
   stl_allocate(plane);
   stl_facet facet1;
   stl_facet facet2;
   stl_vertex tmp;
   float a=0.5;
   tmp.x=a;
   tmp.y=-a;
   tmp.z=0;
   facet1.vertex[0]=tmp;
   facet2.vertex[0]=tmp;
   tmp.y=a;
   facet1.vertex[1]=tmp;
   tmp.x=-a;
   facet1.vertex[2]=tmp;
   facet2.vertex[1]=tmp;
   tmp.y=-a;
   facet2.vertex[2]=tmp;
   facet1.normal.x = 0;
   facet1.normal.y = 0;
   facet1.normal.z = 1;
   facet2.normal.x = 0;
   facet2.normal.y = 0;
   facet2.normal.z = 1;


   plane->facet_start[0] = facet1;
   stl_facet_stats(plane, facet1, 1);
   plane->facet_start[1] = facet2;
   stl_facet_stats(plane, facet2, 0);
   QString name="plane";

   for(QList<MeshObject*>::size_type i = 0; i < count;i++)
   {
            //if(objectList[i]->isActive())
                {
                    float scale=objectList[i]->getDiameter();
                    if(scale<0) scale*=-1;
                    stl_scale(plane,scale*3);
                    //planeMax=0.5*scale*3;
                    MeshObject* item = new MeshObject(plane, name);
                    item->setPlane();
                    //item->setSplitName((int)1);
                    objectList.push_back(item);
                    count+=1;
                    planeShown=true;
                    setPlanePosition();
                    renewListView();
                    reDrawSignal();
                    return;
                }
    }

        
    }
    else
    {
     planeShown=false; 
     for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            //if(objectList[i]->getName()=="plane")
            if(objectList[i]->isPlane())
                {
                    objectList.erase(objectList.begin() + i);
                    //objectList.removeAt(i);
                    delete plane;
                    count--;
                    
                    renewListView();
                    reDrawSignal();
                    return;
                }
            }   
    }
}


void admeshController::setACut(double factor)
{
    /*
    musim translatnout o D
    a rotatnout podle tech dalsich. .prez vypocet
    */
    a_cut = factor;
    setPlanePosition();
  
}
void admeshController::setBCut(double factor)
{
    b_cut = factor;
    setPlanePosition();
}
void admeshController::setCCut(double factor)
{
    c_cut = factor;
    setPlanePosition();
}
void admeshController::setDCut(double factor)
{
    d_cut = factor;
    setPlanePosition();
    /*
        for(QList<MeshObject*>::size_type i = 0; i < count;i++)
    {
        if(objectList[i]->getName()=="plane")
        {
            objectList[i]->center();
            objectList[i]->translate(true, 0, 0, d_cut); 
            reDrawSignal();
            //pushHistory();
            return;
        }
    }
    */
}

void admeshController::setPlanePosition()
{
    //cout<<a_cut<<" "<<b_cut<<" "<<c_cut<<" "<<d_cut<<endl;
        for(QList<MeshObject*>::size_type i = 0; i < count;i++)
    {
        //if(objectList[i]->getName()=="plane")
        if(objectList[i]->isPlane())
        {
            const float PI=3.14159265;
            //objectList[i]->center();
            resetPlane();
            objectList[i]->translate(true, 0, 0, d_cut); 
            double length=sqrt(a_cut*a_cut+b_cut*b_cut+c_cut*c_cut);
            double rotY=acos(c_cut/length);
            double rotZ=atan2(b_cut,a_cut);
            objectList[i]->rotateY(rotY*180/PI);
            objectList[i]->rotateZ(rotZ*180/PI);
            objectList[i]->updateGeometry();


            reDrawSignal();
            //pushHistory();
            return;
        }
    }
}  




void admeshController::cut()
{
    //if(rot != 0.0){
        #ifdef QT_DEBUG
            start_time = clock();
        #endif
        renewList();
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            if(objectList[i]->isActive() && !objectList[i]->isPlane())//objectList[i]->getName()!="plane")
                {
                    std::array<stl_file*,2> cutMesh;
                    bool succes=false;
                    cutMesh=objectList[i]->cut(a_cut,b_cut,c_cut,d_cut,succes);
                    if(succes)
                    {
                        for (int m = 0; m < 2; ++m)
                        {
                            MeshObject* item = new MeshObject(cutMesh[m], objectList[i]->getName());
                            item->setSplitName((int)m);
                            item->setDeselected();
                            objectList.push_back(item);
                        }
                        count+=2;

                        delete objectList[i];
                        objectList.erase(objectList.begin() + i);
                        --count;
                        --i;
                        
                        //cout<<a_cut<<" "<<b_cut<<" "<<c_cut<<" "<<d_cut<<endl;
                        statusBar->setText(QString("Status: Cut succesful."));
                        renewListView();
                    }
                    else
                    {
                        statusBar->setText(QString("Status: Cut failed."));
                    //break;
                    }
                    
                }
                
        }
        //int rotated = selectedCount();
        //if(rotated>0)
        //statusBar->setText(QString(ngettext("Status: %1 mesh CUT along X axis", "Status: %1 meshes rotated along X axis", rotated)).arg(rotated));
        pushHistory();
        #ifdef QT_DEBUG
            cout << "Cut took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
        #endif
    //}
    showPlane();
    showPlane();
    reDrawSignal();
}

void admeshController::rotateX()
{
    if(rot != 0.0){
        #ifdef QT_DEBUG
            start_time = clock();
        #endif
        bool deselected=deselectPlane();
        renewList();
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            if(objectList[i]->isActive())objectList[i]->rotateX(rot);
        }
        int rotated = selectedCount();
        if(rotated>0)statusBar->setText(QString(ngettext("Status: %1 mesh rotated along X axis", "Status: %1 meshes rotated along X axis", rotated)).arg(rotated));
        pushHistory();
        #ifdef QT_DEBUG
            cout << "Rotate X took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
        #endif
        setPlaneSelection(deselected);
    }

    reDrawSignal();
}


void admeshController::rotateY()
{
    if(rot != 0.0){
        #ifdef QT_DEBUG
            start_time = clock();
        #endif
        bool deselected=deselectPlane();   
        renewList();
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            if(objectList[i]->isActive())objectList[i]->rotateY(rot);
        }
        int rotated = selectedCount();
        if(rotated>0)statusBar->setText(QString(ngettext("Status: %1 mesh rotated along Y axis", "Status: %1 meshes rotated along Y axis", rotated)).arg(rotated));
        pushHistory();
        #ifdef QT_DEBUG
            cout << "Rotate Y took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
        #endif
        setPlaneSelection(deselected);  
    }
   
    reDrawSignal();
}

void admeshController::rotateZ()
{
    if(rot != 0.0){
        #ifdef QT_DEBUG
            start_time = clock();
        #endif
        bool deselected=deselectPlane();  
        renewList();
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            if(objectList[i]->isActive() && !objectList[i]->isPlane())objectList[i]->rotateZ(rot);
        }
        int rotated = selectedCount();
        if(rotated>0)statusBar->setText(QString(ngettext("Status: %1 mesh rotated along Z axis", "Status: %1 meshes rotated along Z axis", rotated)).arg(rotated));
        pushHistory();
        #ifdef QT_DEBUG
            cout << "Rotate Z took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
        #endif
        setPlaneSelection(deselected);
    }

    reDrawSignal();
}

void admeshController::setXTranslate(double factor)
{
    x_translate = factor;
}

void admeshController::setYTranslate(double factor)
{
    y_translate = factor;
}

void admeshController::setZTranslate(double factor)
{
    z_translate = factor;
}

void admeshController::setRelativeTranslate()
{
    if(rel_translate) rel_translate = false;
    else rel_translate = true;
}

void admeshController::translate()
{
    if(!rel_translate || x_translate != 0.0 || y_translate != 0.0 || z_translate != 0.0){
        #ifdef QT_DEBUG
            start_time = clock();
        #endif
        bool deselected=deselectPlane(); 
        renewList();
        for(QList<MeshObject*>::size_type i = 0; i < count;i++){
            if(objectList[i]->isActive())
                objectList[i]->translate(rel_translate, x_translate, y_translate, z_translate);
        }
        int translated = selectedCount();
        if(translated>0 && rel_translate)statusBar->setText(QString(ngettext("Status: %1 mesh translated relatively to position", "Status: %1 meshes translated relatively to position", translated)).arg(translated));
        else if(translated>0)statusBar->setText(QString(ngettext("Status: %1 mesh translated to origin", "Status: %1 meshes translated to origin", translated)).arg(translated));
        pushHistory();
        #ifdef QT_DEBUG
            cout << "Translate took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
        #endif
        setPlaneSelection(deselected);
    }

    reDrawSignal();
}

void admeshController::center()
{
    renewList();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive())objectList[i]->center();
    }
    int centered = selectedCount();
    statusBar->setText(QString(ngettext("Status: %1 mesh centered", "Status: %1 meshes centered", centered)).arg(centered));
    pushHistory();
    reDrawSignal();
}

void admeshController::snapZ()
{
    bool deselected=deselectPlane();
    renewList();
    int snapped = selectedCount();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive() )objectList[i]->snapZ();
    }

    statusBar->setText(QString(ngettext("Status: %1 mesh snapped to Z", "Status: %1 meshes snapped to Z", snapped)).arg(snapped));
    pushHistory();
    setPlaneSelection(deselected); 
    reDrawSignal();
}

void admeshController::reverseAll()
{
    renewList();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive())objectList[i]->reverseAll();
    }
    if(selectedCount()>0)statusBar->setText(_("Status: Orientation of facets has been reversed."));
    pushHistory();
    reDrawSignal();
}

void admeshController::setFixAllFlag()
{
    fixall_flag = !fixall_flag;
}

void admeshController::setExactFlag()
{
    exact_flag = !exact_flag;
}

void admeshController::setToleranceFlag()
{
    tolerance_flag = !tolerance_flag;
}

void admeshController::setTolerance(double val)
{
    tolerance = val;
}

void admeshController::setIncrementFlag()
{
    increment_flag = !increment_flag;
}

void admeshController::setIncrement(double val)
{
    increment = val;
}

void admeshController::setNearbyFlag()
{
    nearby_flag = !nearby_flag;
}

void admeshController::setIterations(int val)
{
    iterations = val;
}

void admeshController::setRemoveUnconnectedFlag()
{
    remove_unconnected_flag = !remove_unconnected_flag;
}

void admeshController::setFillHolesFlag()
{
    fill_holes_flag = !fill_holes_flag;
}

void admeshController::setNormalDirFlag()
{
    normal_directions_flag = !normal_directions_flag;
}

void admeshController::setNormalValFlag()
{
    normal_values_flag = !normal_values_flag;
}

bool admeshController::deselectPlane()
{
    for(QList<MeshObject*>::size_type i = count-1; i >=0;i--)
        {
            if(objectList[i]->isPlane())
            {
                if(objectList[i]->isSelected())    
                {
                    objectList[i]->setDeselected();
                    return true;  
                }
                else return false;
            }
        }
        return false;
}
void admeshController::setPlaneSelection(bool select)
{
    for(QList<MeshObject*>::size_type i = count-1; i >=0;i--)
        {
            if(objectList[i]->isPlane())
            {
                  if(select) objectList[i]->setSelected();
                  else       objectList[i]->setDeselected();
            }
        } 
}

void admeshController::repair()
{
    #ifdef QT_DEBUG
        start_time = clock();
    #endif
     bool deselected=deselectPlane();    
    renewList();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive())objectList[i]->repair(fixall_flag,
                              exact_flag,
                              tolerance_flag,
                              tolerance,
                              increment_flag,
                              increment,
                              nearby_flag,
                              iterations,
                              remove_unconnected_flag,
                              fill_holes_flag,
                              normal_directions_flag,
                              normal_values_flag,
                              false);
    }
    int repaired = selectedCount();
    if(repaired>0)statusBar->setText(QString(ngettext("Status: %1 mesh repaired", "Status: %1 meshes repaired", repaired)).arg(repaired));
    pushHistory();
    #ifdef QT_DEBUG
        cout << "Repair took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
    #endif
        for(QList<MeshObject*>::size_type i = count-1; i >=0;i--)
        {
             if(objectList[i]->isPlane()) {objectList[i]->setSelected(); break;}
        }  
    setPlaneSelection(deselected);
    reDrawSignal();
}

void stl_merge(stl_file *stl, stl_file *stl_to_merge) {
    int num_facets_so_far;
    if (stl->error) return;
    num_facets_so_far = stl->stats.number_of_facets;
    stl->stats.type=stl_to_merge->stats.type;
    stl->stats.number_of_facets+=stl_to_merge->stats.number_of_facets;
    stl_reallocate(stl);
    for(int i=0; i<stl_to_merge->stats.number_of_facets;i++){
        stl->facet_start[num_facets_so_far+i] = stl_to_merge->facet_start[i];
        stl_facet_stats(stl, stl->facet_start[num_facets_so_far+i], 0);
        stl->neighbors_start[num_facets_so_far+i] = stl_to_merge->neighbors_start[i];
    }
    stl->stats.backwards_edges += stl_to_merge->stats.backwards_edges;
    stl->stats.collisions += stl_to_merge->stats.collisions;
    stl->stats.connected_edges += stl_to_merge->stats.connected_edges;
    stl->stats.degenerate_facets += stl_to_merge->stats.degenerate_facets;
    stl->stats.edges_fixed += stl_to_merge->stats.edges_fixed;
    stl->stats.normals_fixed += stl_to_merge->stats.normals_fixed;
    stl->stats.number_of_parts += stl_to_merge->stats.number_of_parts;
    stl->stats.facets_reversed += stl_to_merge->stats.facets_reversed;
    stl->stats.size.x = stl->stats.max.x - stl->stats.min.x;
    stl->stats.size.y = stl->stats.max.y - stl->stats.min.y;
    stl->stats.size.z = stl->stats.max.z - stl->stats.min.z;
    stl->stats.bounding_diameter = sqrt(
                                     stl->stats.size.x * stl->stats.size.x +
                                     stl->stats.size.y * stl->stats.size.y +
                                     stl->stats.size.z * stl->stats.size.z
                                     );
}

void admeshController::merge()
{
    for(QList<MeshObject*>::size_type i = 0; i < count;i++)
    {
        if(objectList[i]->isPlane() && objectList[i]->isActive())
        {
            QString msg;
            QTextStream(&msg) << _("You cant merge models with cutting plane, unselect it.\n");
            QMessageBox::warning(NULL, _("Warning"), msg);
            return;
        }

    }
    if(selectedCount()<2 ){
        QString msg;
        QTextStream(&msg) << _("Not enough files to merge selected.\n");
        QMessageBox::warning(NULL, _("Warning"), msg);
        return;
    }
    #ifdef QT_DEBUG
        start_time = clock();
    #endif
    renewList();
    QList<MeshObject*>::size_type merged = 0;
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive()){
            merged = i;
            break;
        }
    }
    for(QList<MeshObject*>::size_type i = merged+1; i < count;i++){
        if(objectList[i]->isActive()){
            stl_merge(objectList[merged]->getStlPointer(), objectList[i]->getStlPointer());
            delete objectList[i];
            objectList.erase(objectList.begin() + i);
            --count;
            --i;
        }
    }
    stl_calculate_volume(objectList[merged]->getStlPointer());
    objectList[merged]->mergedFilename();
    objectList[merged]->updateGeometry();
    renewListView();
    statusBar->setText(_("Status: meshes merged"));
    pushHistory();
    #ifdef QT_DEBUG
        cout << "Merge took " << (clock()-start_time)/(double)CLOCKS_PER_SEC << "s" << endl;
    #endif
    reDrawSignal();
}

void admeshController::split(){
    vector<stl_file*> result;
    int added = 0, notSplit = 0, split=0;
    renewList();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive()){
            result = stl_split(objectList[i]->getStlPointer());
            if(result.size()>1){
                for(vector<stl_file*>::size_type j = 0; j < result.size(); j++){
                    MeshObject* item = new MeshObject(result[j], objectList[i]->getName());
                    item->setSplitName((int)j);
                    objectList.push_back(item);
                    added++;
                }
               
                delete objectList[i];
                objectList.erase(objectList.begin() + i);
                --count;
                --i;
                split++;
            }else{
                notSplit++;
            }
        }
    }
    count += added;
    renewListView();
    QString status = "Status: ";
    if(split) status += QString(ngettext("%1 mesh split into %2 meshes. ", "%1 meshes split into %2 meshes. ", split)).arg(split).arg(added);
    if(notSplit) status += QString(ngettext("%1 mesh contains only 1 shell and cannot be split", "%1 meshes contain only 1 shell and cannot be split", notSplit)).arg(notSplit);
    statusBar->setText(status);
    pushHistory();
    reDrawSignal();
}

void admeshController::duplicate()
{   bool deselected=deselectPlane();
    renewList();
    int added = 0;
    int orig = selectedCount();
    for(QList<MeshObject*>::size_type i = 0; i < count;i++){
        if(objectList[i]->isActive() && !objectList[i]->isPlane()){
            MeshObject *duplicated = new MeshObject(*objectList[i]);
            duplicated->setDuplicatedName();
            objectList.push_back(duplicated);
            added++;
        }
    }
    count += added;
    renewListView();
    statusBar->setText(QString(ngettext("Status: %1 mesh duplicated", "Status: %1 meshes duplicated", orig)).arg(orig));
    pushHistory();
    setPlaneSelection(deselected);
    reDrawSignal();
}
