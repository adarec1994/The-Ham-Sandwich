using System;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;

namespace ProjectWS.Engine.Database
{
    public class FieldCache
    {
        public FieldInfo Field;
        public int ArraySize;
        public bool IsIndex;
        public bool IsNonInline;
        public bool IsArray;
    }

    public class FieldCache<T, V> : FieldCache
    {
        public readonly Action<T, V> Setter;
        public readonly Func<T, V> Getter;

        public FieldCache(FieldInfo field)
        {
            Field = field;
            IsArray = field.FieldType.IsArray;

            if (IsArray)
            {
                ArraySizeAttribute atr = (ArraySizeAttribute)field.GetCustomAttribute(typeof(ArraySizeAttribute));

                if (atr == null)
                    throw new Exception(typeof(T).Name + "." + field.Name + " missing ArraySizeAttribute");

                ArraySize = atr.Size;
            }

            Setter = field.GetSetter<T, V>();
            Getter = field.GetGetter<T, V>();
        }
    }

    public class FieldsCache<T>
    {
        private static readonly FieldCache[] fieldsCache;

        static FieldsCache()
        {
            FieldInfo[] fields = typeof(T).GetFields(BindingFlags.Public | BindingFlags.Instance).OrderBy(f => f.MetadataToken).ToArray();

            fieldsCache = new FieldCache[fields.Length];

            for (int i = 0; i < fields.Length; i++)
                fieldsCache[i] = (FieldCache)Activator.CreateInstance(typeof(FieldCache<,>).MakeGenericType(typeof(T), fields[i].FieldType), fields[i]);
        }

        public static FieldCache[] Cache => fieldsCache;
    }

    public static class FieldExtensions
    {
        public static Action<T, V> GetSetter<T, V>(this FieldInfo fieldInfo)
        {
            var paramExpression = Expression.Parameter(typeof(T));
            var fieldExpression = Expression.Field(paramExpression, fieldInfo);
            var valueExpression = Expression.Parameter(fieldInfo.FieldType);
            var assignExpression = Expression.Assign(fieldExpression, valueExpression);

            return Expression.Lambda<Action<T, V>>(assignExpression, paramExpression, valueExpression).Compile();
        }

        public static Func<T, V> GetGetter<T, V>(this FieldInfo fieldInfo)
        {
            var paramExpression = Expression.Parameter(typeof(T));
            var fieldExpression = Expression.Field(paramExpression, fieldInfo);

            return Expression.Lambda<Func<T, V>>(fieldExpression, paramExpression).Compile();
        }
    }
}