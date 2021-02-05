#version 330 core

out vec4 FragColor;

in vec2 uv;

uniform int highlight_key;

void main() {

  if(highlight_key == 2) {
    FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

  } else if (highlight_key == 1) {
    //    FragColor = vec4(1.0f, 0.5f, 0.0f, 1.0f);
    FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    //FragColor = vec4(0.812,0.216,0.129, 1.0f);
    
  } else {

    //FragColor = vec4(0.1f, 0.7f, 0.1f, 1.0f);
    //FragColor = vec4(0.945,0.953,0.765, 1.0f);

    //    FragColor = vec4(0.945,0.953,0.808, 1.0);
    //    FragColor = vec4(0.553 * 3. ,0.137 * 3. ,0.059 * 3., 1.); //red
    //    FragColor = vec4(0., 0., 0., 1.); //black
    FragColor = vec4(0.961,0.745,0.255, 1.0f); //yellow/orange
    //    FragColor = vec4(uv.x, uv.y, 0., 1.);
  }
}
