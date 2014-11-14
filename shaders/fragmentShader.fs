    #version 330
    
    layout(location = 0) out vec4 outputColor;

    in vec3 Normal;

    void main()
    {
       outputColor = vec4(abs(Normal.x),abs(Normal.y),abs(Normal.z), 1.0f);
       //outputColor = vec4(0.7, 0.0f, 0.0f, 1.0f);
    }