#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fmt/core.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <chrono>
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
    ELT& operator()(unsigned x, unsigned y) {
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
void compute_image(Image<T> & img) {
    auto time = std::chrono::steady_clock::now();
    std::chrono::duration<float> m = time.time_since_epoch();
    float int_part;
    float fract_part = modff(m.count(), &int_part);
    float extra = fract_part * 2 * M_PIf32;
    float fx = 2 * M_PIf32 / ((float) img.width() - 1);
    float fy = 2 * M_PIf32 / ((float) img.height() - 1);
    for (int x = 0; x < img.width(); ++x) {
        for (int y = 0; y < img.height(); ++y) {
            auto& f = img(x, y);
            //float *f = &data[3 * (img.width() * y + x)];
            f[0] = 0.5f + 0.5 * sinf(x * fx + extra );
            f[1] = 0.5f + 0.5 * sinf(y * fy + extra);
            f[2] = 0.5f + 0.5 * cosf(y * fy + extra - M_PIf32);
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

  int width = 256;
  int height = 256;
  Image<std::array<float, 3>> img(width, height);

  compute_image(img);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, img.begin());
  glGenerateMipmap(GL_TEXTURE_2D);

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
  while (!glfwWindowShouldClose(window)) {
    timer.stop();
    float delta_time_s = timer.seconds_interval();
    glfwPollEvents();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);

    glBindTexture(GL_TEXTURE_2D, texture);
    //compute_texture(width, height, data);
    compute_image(img);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, data.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, img.begin());
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