using System;
using System.Collections.Generic;
using System.Formats.Asn1;
using System.Linq;
using System.Text;
using System.Text.Json.Serialization;
using System.Text.Json;
using System.Threading.Tasks;
using System.Globalization;

namespace MathUtils
{
    public class Vector2JsonConverter : JsonConverter<Vector2>
    {
        public override Vector2 Read(
            ref Utf8JsonReader reader,
            Type typeToConvert,
            JsonSerializerOptions options) => Vector2.FromString(reader.GetString()!);

        public override void Write(
            Utf8JsonWriter writer,
            Vector2 vector,
            JsonSerializerOptions options) => writer.WriteStringValue(vector.ToString());
    }

    public class Vector2iJsonConverter : JsonConverter<Vector2i>
    {
        public override Vector2i Read(
            ref Utf8JsonReader reader,
            Type typeToConvert,
            JsonSerializerOptions options) => Vector2i.FromString(reader.GetString()!);

        public override void Write(
            Utf8JsonWriter writer,
            Vector2i vector,
            JsonSerializerOptions options) => writer.WriteStringValue(vector.ToString());
    }

    public class Vector3JsonConverter : JsonConverter<Vector3>
    {
        public override Vector3 Read(
            ref Utf8JsonReader reader,
            Type typeToConvert,
            JsonSerializerOptions options) => Vector3.FromString(reader.GetString()!);

        public override void Write(
            Utf8JsonWriter writer,
            Vector3 vector,
            JsonSerializerOptions options) => writer.WriteStringValue(vector.ToString());
    }

    public class QuaternionJsonConverter : JsonConverter<Quaternion>
    {
        public override Quaternion Read(
            ref Utf8JsonReader reader,
            Type typeToConvert,
            JsonSerializerOptions options) => Quaternion.FromString(reader.GetString()!);

        public override void Write(
            Utf8JsonWriter writer,
            Quaternion vector,
            JsonSerializerOptions options) => writer.WriteStringValue(vector.ToString());
    }
}
