#version 330

  in vec3 fragPos;
  in vec3 fragNormal;
  in vec2 fragTexCoord;
  in vec4 fragPosLight;

  uniform sampler2D texture0;
  uniform vec4 colDiffuse;

  uniform sampler2D shadowMap;
  uniform vec3 lightDir;

  out vec4 finalColor;

  float ShadowFactor(vec4 lightSpacePos)
  {
      vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
      projCoords = projCoords * 0.5 + 0.5;

      if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
          projCoords.y < 0.0 || projCoords.y > 1.0 ||
          projCoords.z > 1.0)
      {
          return 0.0;
      }

      float closestDepth = texture(shadowMap, projCoords.xy).r;
      float currentDepth = projCoords.z;

      float bias = max(0.0025 * (1.0 - dot(normalize(fragNormal), normalize(-lightDir))), 0.0005);
      return currentDepth - bias > closestDepth ? 0.6 : 0.0;
  }

  void main()
  {
      vec3 base = colDiffuse.rgb * texture(texture0, fragTexCoord).rgb;

      float diff = max(dot(normalize(fragNormal), normalize(-lightDir)), 0.0);
      float shadow = ShadowFactor(fragPosLight);

      float lighting = 0.25 + (1.0 - shadow) * diff;
      finalColor = vec4(base * lighting, colDiffuse.a);
  }