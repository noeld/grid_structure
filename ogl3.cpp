#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fmt/core.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <functional>
#include <memory>
#include <chrono>
#include <random>
#include <ratio>
#include <thread>
#include <tuple>


class Timer {
public:
    using clock_type = std::chrono::high_resolution_clock;
    using time_point = clock_type::time_point;

    static time_point now() noexcept { return clock_type::now(); }

    explicit Timer() : start_ { now() }, stop_ { start_ }, last_stop_{ start_ }
    {}

    void start() { start_ = now(); }
    void stop() {
        last_stop_  = stop_;
        stop_ = now();
    }

    float seconds_interval() const {
        return std::chrono::duration<float>(stop_ - last_stop_).count();
    }
    float seconds_total() const {
        return std::chrono::duration<float>(stop_ - start_).count();
    }
protected:
    time_point start_, stop_, last_stop_;
};

constexpr int window_width = 1024;
constexpr int window_height = 768;
constexpr char const * window_title = "ogl2";

template<typename ELT>
class Image {
public:
    using value_type = ELT;
    explicit Image(unsigned width, unsigned height)
    : width_{width}, height_{height}, len_{width * height}
    , data_{std::make_unique<value_type[]>(width * height)}
    {}
    ELT& operator()(unsigned x, unsigned y) noexcept {
        return data_[y * width_ + x];
    }
    ELT const& operator()(unsigned x, unsigned y) const noexcept {
        return data_[y * width_ + x];
    }
    std::span<ELT> row(unsigned y) {
        return std::span(data_.get() + y * width_, width_);
    }
    ELT* row_ptr(unsigned y) {
        return &data_[y * width_];
    }
    auto begin() { return data_.get(); }
    auto end() { return data_.get() + len_ ; }
    auto size() const noexcept { return len_; }
    auto width() const noexcept { return width_; }
    auto height() const noexcept { return height_; }
protected:
    unsigned const width_, height_, len_;
    std::unique_ptr<ELT[]> data_;
};

void resize(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

template<typename T>
void compute_image(Image<T> const * src, Image<T>* tgt) {
    auto const & s = *src;
    auto& t = *tgt;
    unsigned border = 4;
    for(unsigned y = border; y < src->height() - border; ++y) {
        for(unsigned x = border; x < src->width() - border; ++x) {
            float avg = 0.f;
            for(unsigned by = y - border; by < y + border + 1; ++by) {
                for(unsigned bx = x - border; bx < x + border + 1; ++bx) {
                    avg += s(bx, by);
                }
            }
            avg /= (2*border+1) * (2*border+1);
            t(x, y) = s(x, y) + (avg - s(x, y)) * 0.05f;
        }
    }

}

int main(int argc, char const *argv[]) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(window_width, window_height,
                                        window_title, nullptr, nullptr);
  if (window == nullptr) {
    fmt::println(stderr, "Fehler beim Erstellen des Fensters!\n");
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, resize);

  if (glewInit() != GLEW_OK) {
    fmt::println(stderr, "Fehler beim Initialisieren von GLEW");
    glfwTerminate();
    return -1;
  }

  unsigned texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int width = 1024;
  int height = 1024;
  Image<float> img1(width, height);
  Image<float> img2(width, height);
  auto mk_randomizer = [](float max){
      std::mt19937_64 gen(std::random_device{}());
      std::uniform_real_distribution<float> dist(0, max);
      auto rnd = std::bind(dist, gen);
      return [rnd]() mutable { return rnd(); };
  };
  auto fgen = mk_randomizer(1.0f);
  std::generate(img1.begin(), img1.end(), fgen);
  std::copy(img1.begin(), img1.end(), img2.begin());

  // Erstelle Vertex Array Object (VAO) und Vertex Buffer Object (VBO)
  unsigned int VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  float vertices[] = {
      // Positionen     // Textur-Koordinaten
      1.0f,  1.0f,  0.0f, 1.0f, 1.0f, // Oben rechts
      1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // Unten rechts
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // Unten links
      -1.0f, 1.0f,  0.0f, 0.0f, 1.0f  // Oben links
  };
  unsigned int indices[] = {
      0, 1, 3, // Erstes Dreieck
      1, 2, 3  // Zweites Dreieck
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Erstelle Shader
  const char *vertexShaderSource = "#version 330 core\n"
                                   "layout (location = 0) in vec3 aPos;\n"
                                   "layout (location = 1) in vec2 aTexCoord;\n"
                                   "out vec2 TexCoord;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   gl_Position = vec4(aPos, 1.0);\n"
                                   "   TexCoord = aTexCoord;\n"
                                   "}\0";
  const char *fragmentShaderSource =
      "#version 330 core\n"
      "in vec2 TexCoord;\n"
      "out vec4 FragColor;\n"
      "uniform sampler2D ourTexture;\n"
      "void main()\n"
      "{\n"
      "   FragColor = texture(ourTexture, TexCoord);\n"
      "}\0";
  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  unsigned int shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  fmt::println(stderr, "glDeleteShader");
  Timer timer;

  Image<float> *src = &img1;
  Image<float> *tgt = &img2;

  while (!glfwWindowShouldClose(window)) {
    timer.stop();
    float delta_time_s = timer.seconds_interval();
    glfwPollEvents();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);

    glBindTexture(GL_TEXTURE_2D, texture);
    //compute_texture(width, height, data);
    compute_image(src, tgt);
    //fmt::println(stderr, "compute_image");
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, data.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RED, GL_FLOAT, tgt->begin());
    //glGenerateMipmap(GL_TEXTURE_2D);
    //fmt::println(stderr, "glTexImage2D");
    std::swap(src, tgt);
    glBindVertexArray(VAO);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

    glfwSwapBuffers(window);
    float rest_time = 1.f/60.f - delta_time_s;
    if (rest_time > 0.f)
        std::this_thread::sleep_for(std::chrono::duration<float>(rest_time));
  }
  glfwTerminate();

  return 0;
}