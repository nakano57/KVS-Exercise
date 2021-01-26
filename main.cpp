#include <kvs/Application>
#include <kvs/Screen>
#include <kvs/CheckBox>
#include <kvs/Slider>
#include <kvs/PolygonObject>
#include <kvs/PolygonImporter>
#include <kvs/StructuredVolumeObject>
#include <kvs/HydrogenVolumeData>
#include <kvs/Isosurface>
#include <kvs/StochasticPolygonRenderer>
#include <kvs/Scene>
#include <kvs/ObjectManager>
#include <kvs/RendererManager>
#include <iostream>

#include <kvs/StructuredVolumeImporter>
#include <kvs/StructuredVectorToScalar>

/*===========================================================================*/
/**
 *  @brief  LOD check box.
 */
/*===========================================================================*/
class LODCheckBox : public kvs::CheckBox
{
public:
    LODCheckBox(kvs::Screen *screen) : kvs::CheckBox(screen)
    {
        setMargin(10);
        setCaption("Level-of-Detail");
    }

    void stateChanged()
    {
        typedef kvs::StochasticPolygonRenderer Renderer;
        kvs::Scene *scene = kvs::Screen::DownCast(screen())->scene();
        Renderer *renderer = static_cast<Renderer *>(scene->rendererManager()->renderer("Renderer"));
        renderer->setEnabledLODControl(state());
        screen()->redraw();
    }
};

/*===========================================================================*/
/**
 *  @brief  Opacity slider.
 */
/*===========================================================================*/
class OpacitySlider : public kvs::Slider
{
public:
    OpacitySlider(kvs::Screen *screen) : kvs::Slider(screen)
    {
        setWidth(150);
        setMargin(10);
        setCaption("Opacity");
    }

    void valueChanged()
    {
        typedef kvs::PolygonObject Object;
        kvs::Scene *scene = kvs::Screen::DownCast(screen())->scene();
        Object *object1 = Object::DownCast(scene->objectManager()->object("Polygon"));
        Object *object2 = new Object();
        object2->shallowCopy(*object1);
        object2->setName("Polygon");
        object2->setOpacity(int(value() * 255 + 0.5));
        scene->objectManager()->change("Polygon", object2);
    }
};

/*===========================================================================*/
/**
 *  @brief  Repetition slider.
 */
/*===========================================================================*/
class RepetitionSlider : public kvs::Slider
{
public:
    RepetitionSlider(kvs::Screen *screen) : kvs::Slider(screen)
    {
        setWidth(150);
        setMargin(10);
        setCaption("Repetition");
    }

    void valueChanged()
    {
        typedef kvs::StochasticPolygonRenderer Renderer;
        kvs::Scene *scene = kvs::Screen::DownCast(screen())->scene();
        Renderer *renderer = static_cast<Renderer *>(scene->rendererManager()->renderer("Renderer"));
        renderer->setRepetitionLevel(int(value() + 0.5));
        screen()->redraw();
    }
};

/*===========================================================================*/
/**
 *  @brief  Isosurface slider.
 */
/*===========================================================================*/
class
    IsosurfaceSlider : public kvs::Slider
{
private:
    double min_value, max_value;
    kvs::StructuredVolumeObject *scalar_volume_object;

public:
    IsosurfaceSlider(kvs::Screen *screen, double min, double max, kvs::StructuredVolumeObject *o) : kvs::Slider(screen)
    {
        min_value = min;
        max_value = max;
        scalar_volume_object = o;
        setWidth(150);
        setMargin(10);
        setCaption("Isosurface");
    }

    void valueChanged()
    {
        typedef kvs::PolygonObject Object;
        kvs::Scene *scene = kvs::Screen::DownCast(screen())->scene();
        Object *object1 = Object::DownCast(scene->objectManager()->object("Polygon"));
        kvs::Isosurface *object2 = new kvs::Isosurface();
        object2->shallowCopy(*object1);
        object2->setName("Polygon");
        object2->setIsolevel(double((min_value + max_value) * value()));
        object2->exec(scalar_volume_object);
        object2->setOpacities(object1->opacities());
        scene->objectManager()->change("Polygon", object2);
    }
};

int main(int argc, char **argv)
{
    // Application and Screen. KVSを画面に表示するためのモノ. 必須
    kvs::Application app(argc, argv);
    kvs::Screen screen(&app);
    screen.setBackgroundColor(kvs::RGBColor::White()); // 背景を白に.
    //screen.show();

    kvs::StructuredVolumeObject *vector_volume_object = new kvs::StructuredVolumeImporter(std::string(argv[1])); // .kvsml形式でデータが記述されているものはデータに適したImporterクラスを使うことでデータの読み込み、オブジェクトへの変換が行える.
    kvs::StructuredVolumeObject *scalar_volume_object = new kvs::StructuredVectorToScalar(vector_volume_object); // ベクトルデータをスカラデータに変換. ベクトルの大きさが計算される.

    double min_value = scalar_volume_object->minValue(); // スカラデータの中から最小値を求める.
    double max_value = scalar_volume_object->maxValue(); // 同様に最大値
    double isolevel = (min_value + max_value) * 0.1;     // 等値面を生成のための閾値を設定する.0.1のところを変えると出力が変わってくる

    kvs::PolygonObject::NormalType n = kvs::PolygonObject::VertexNormal;
    kvs::Isosurface *isosurface = new kvs::Isosurface(scalar_volume_object, n); // 等値面生成

    isosurface->setName("Polygon");
    isosurface->setOpacity(128);
    isosurface->print(std::cout);
    isosurface->setIsolevel(isolevel);
    isosurface->exec(scalar_volume_object);

    // レンダラーの設定
    auto *renderer = new kvs::StochasticPolygonRenderer();
    renderer->setRepetitionLevel(50);
    renderer->enableLODControl();
    renderer->setName("Renderer");
    screen.registerObject(isosurface, renderer); // オブジェクトとレンダラーを登録
    screen.create();

    LODCheckBox checkbox(&screen);
    checkbox.setPosition(0, 0);
    checkbox.setState(true);
    checkbox.show();

    OpacitySlider opacity(&screen);
    opacity.setPosition(checkbox.x(), checkbox.y() + checkbox.height());
    opacity.setValue(1);
    opacity.setRange(0, 1);
    opacity.show();

    RepetitionSlider repetition(&screen);
    repetition.setPosition(opacity.x(), opacity.y() + opacity.height());
    repetition.setValue(50);
    repetition.setRange(1, 100);
    repetition.show();

    IsosurfaceSlider isoslider(&screen, min_value, max_value, scalar_volume_object);
    isoslider.setPosition(repetition.x(), repetition.y() + repetition.height());
    isoslider.setValue(0.1);
    isoslider.setRange(0, 1);
    isoslider.show();

    return app.run();
}
